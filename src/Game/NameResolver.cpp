#include "NameResolver.h"
#include "AddressManager.h"
#include "../Utils/MemorySafety.h"
#include "../Utils/StringHelpers.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <windows.h>

namespace kx {
    namespace NameResolver {

        // Forward declarations in the correct namespace scope
        void ProcessCompletedNameRequests();
        void RequestNameForAgent(void* agentPtr, uint8_t type);

        namespace {

            using GetCodedName_t = void* (__fastcall*)(void* thisPtr);
            using DecodeGameText_t = void(__fastcall*)(void* codedText, void* callback, void* ctx);

            struct PendingRequest {
                void* agentPtr = nullptr;
                std::string result;
            };

            std::atomic<uint64_t> s_nextRequestId{1};
            std::unordered_map<uint64_t, PendingRequest> s_pendingRequests;
            std::mutex s_requestsMutex;

            std::unordered_map<void*, std::string> s_nameCache;
            std::mutex s_nameCacheMutex;

            void* GetCodedNamePointerSEH(void* agentPtr, uint8_t type) {
                __try {
                    auto vtable = *reinterpret_cast<uintptr_t**>(agentPtr);
                    if (!SafeAccess::IsMemorySafe(vtable)) {
                        return nullptr;
                    }

                    const auto index = (type == 0) ? 57 : 8;
                    auto pGetCodedName = reinterpret_cast<GetCodedName_t>(vtable[index]);
                    if (!SafeAccess::IsMemorySafe(reinterpret_cast<void*>(pGetCodedName))) {
                        return nullptr;
                    }

                    return pGetCodedName(agentPtr);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return nullptr;
                }
            }

            bool CallDecodeTextSEH(DecodeGameText_t decodeFn, void* codedName, void* callback, void* ctx) {
                __try {
                    decodeFn(codedName, callback, ctx);
                    return true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return false;
                }
            }

            void __fastcall DecodeNameCallback(void* ctx, wchar_t* decodedText) {
                if (!ctx || !decodedText || decodedText[0] == L'\0') {
                    return;
                }

                std::wstring stableCopy(decodedText);
                std::string utf8Name = StringHelpers::WCharToUTF8String(stableCopy.c_str());
                if (utf8Name.empty()) {
                    return;
                }

                std::lock_guard<std::mutex> lock(s_requestsMutex);
                const auto requestId = reinterpret_cast<uint64_t>(ctx);
                auto it = s_pendingRequests.find(requestId);
                if (it != s_pendingRequests.end()) {
                    it->second.result = std::move(utf8Name);
                }
            }

        } // namespace

        void CacheNamesForAgents(const std::unordered_map<void*, uint8_t>& agentPointers) {
            ProcessCompletedNameRequests();

            for (const auto& [agentPtr, type] : agentPointers) {
                if (!agentPtr) {
                    continue;
                }

                bool alreadyProcessed = false;
                {
                    std::lock_guard<std::mutex> cacheLock(s_nameCacheMutex);
                    alreadyProcessed = (s_nameCache.find(agentPtr) != s_nameCache.end());
                }

                if (alreadyProcessed) {
                    continue;
                }

                {
                    std::lock_guard<std::mutex> requestLock(s_requestsMutex);
                    for (const auto& [id, pending] : s_pendingRequests) {
                        if (pending.agentPtr == agentPtr) {
                            alreadyProcessed = true;
                            break;
                        }
                    }
                }

                if (!alreadyProcessed) {
                    RequestNameForAgent(agentPtr, type);
                }
            }
        }

        std::string GetCachedName(void* agentPtr) {
            if (!agentPtr) {
                return {};
            }

            std::lock_guard<std::mutex> lock(s_nameCacheMutex);
            auto it = s_nameCache.find(agentPtr);
            if (it != s_nameCache.end()) {
                return it->second;
            }
            return {};
        }

        void ClearNameCache() {
            {
                std::lock_guard<std::mutex> cacheLock(s_nameCacheMutex);
                s_nameCache.clear();
            }

            {
                std::lock_guard<std::mutex> requestLock(s_requestsMutex);
                s_pendingRequests.clear();
            }
        }

        std::string GetNameFromAgent(void* agentPtr) {
            return GetCachedName(agentPtr);
        }

        void ProcessCompletedNameRequests() {
            std::vector<std::pair<void*, std::string>> completed;
            {
                std::lock_guard<std::mutex> lock(s_requestsMutex);
                for (auto it = s_pendingRequests.begin(); it != s_pendingRequests.end();) {
                    if (!it->second.result.empty()) {
                        completed.emplace_back(it->second.agentPtr, std::move(it->second.result));
                        it = s_pendingRequests.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            if (completed.empty()) {
                return;
            }

            std::lock_guard<std::mutex> cacheLock(s_nameCacheMutex);
            for (auto& entry : completed) {
                s_nameCache[entry.first] = std::move(entry.second);
            }
        }

        void RequestNameForAgent(void* agentPtr, uint8_t type) {
            if (!agentPtr || !SafeAccess::IsVTablePointerValid(agentPtr) || !AddressManager::GetContextCollectionPtr()) {
                return;
            }

            auto decodeFn = reinterpret_cast<DecodeGameText_t>(AddressManager::GetDecodeTextFunc());
            if (!decodeFn) {
                return;
            }

            void* codedName = GetCodedNamePointerSEH(agentPtr, type);
            if (!codedName) {
                return;
            }

            const uint64_t requestId = s_nextRequestId.fetch_add(1, std::memory_order_relaxed);

            {
                std::lock_guard<std::mutex> lock(s_requestsMutex);
                s_pendingRequests.emplace(requestId, PendingRequest{agentPtr, {}});
            }

            const bool success = CallDecodeTextSEH(
                decodeFn,
                codedName,
                reinterpret_cast<void*>(&DecodeNameCallback),
                reinterpret_cast<void*>(requestId));

            if (!success) {
                std::lock_guard<std::mutex> lock(s_requestsMutex);
                s_pendingRequests.erase(requestId);
            }
        }

    } // namespace NameResolver
} // namespace kx
