#include "Hooks.h"

#include <windows.h> // For __try/__except
#include <unordered_map>

#include "../Core/Config.h"      // For GW2AL_BUILD define
#include "../Utils/DebugLogger.h"
#include "../Utils/SafeIterators.h"
#include "../Utils/MemorySafety.h"
#include "../Game/AddressManager.h"
#include "../Game/NameResolver.h"
#include "../Game/ReClassStructs.h"
#include "AppState.h"
#include "D3DRenderHook.h"
#include "HookManager.h"

namespace kx {

    namespace Hooking {

        typedef void(__fastcall* GameThreadUpdateFunc)(void*, int);
        GameThreadUpdateFunc pOriginalGameThreadUpdate = nullptr;

        void* GetContextCollection_SEH() {
            using GetContextCollectionFn = void* (*)();

            const uintptr_t funcAddr = AddressManager::GetContextCollectionFunc();
            if (!funcAddr) {
                return nullptr;
            }

            auto getContextCollection = reinterpret_cast<GetContextCollectionFn>(funcAddr);

            __try {
                return getContextCollection();
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return nullptr;
            }
        }

        void __fastcall DetourGameThread(void* pInst, int frame_time) {
            static int frameCounter = 0;
            if (++frameCounter >= 300) {
                frameCounter = 0;
                NameResolver::ClearNameCache();
            }

            void* pContextCollection = GetContextCollection_SEH();
            AddressManager::SetContextCollectionPtr(pContextCollection);

            if (pContextCollection && SafeAccess::IsMemorySafe(pContextCollection)) {
                std::unordered_map<void*, uint8_t> agentPointers;
                agentPointers.reserve(512);

                ReClass::ContextCollection ctxCollection(pContextCollection);

                ReClass::ChCliContext charContext = ctxCollection.GetChCliContext();
                if (charContext.data()) {
                    SafeAccess::CharacterList characterList(charContext);
                    for (const auto& character : characterList) {
                        if (character.data()) {
                            // Cast away constness for storage as identifier only
                            agentPointers.emplace(const_cast<void*>(character.data()), static_cast<uint8_t>(0));
                        }
                    }
                }

                ReClass::GdCliContext gadgetContext = ctxCollection.GetGdCliContext();
                if (gadgetContext.data()) {
                    SafeAccess::GadgetList gadgetList(gadgetContext);
                    for (const auto& gadget : gadgetList) {
                        if (gadget.data()) {
                            agentPointers.emplace(const_cast<void*>(gadget.data()), static_cast<uint8_t>(1));
                        }
                    }
                }

                if (!agentPointers.empty()) {
                    NameResolver::CacheNamesForAgents(agentPointers);
                }
            }

            if (pOriginalGameThreadUpdate) {
                pOriginalGameThreadUpdate(pInst, frame_time);
            }
        }

    } // namespace Hooking

    bool InitializeHooks() {
        AppState::Get().SetPresentHookStatus(HookStatus::Unknown);

#ifndef GW2AL_BUILD
        if (!Hooking::D3DRenderHook::Initialize()) {
            Hooking::HookManager::Shutdown();
            AppState::Get().SetPresentHookStatus(HookStatus::Failed);
            return false;
        }
#endif

        LOG_INFO("[Hooks] Essential hooks initialized successfully.");
        return true;
    }

    bool InitializeGameThreadHook() {
        static bool s_initialized = false;
        if (s_initialized) {
            LOG_WARN("[Hooks] GameThread hook already initialized, skipping.");
            return true;
        }

        uintptr_t gameThreadFuncAddr = AddressManager::GetGameThreadUpdateFunc();
        if (!gameThreadFuncAddr) {
            LOG_WARN("[Hooks] GameThread hook target not found. Character ESP will be disabled.");
            return false;
        }

        if (!Hooking::HookManager::CreateHook(
            reinterpret_cast<LPVOID>(gameThreadFuncAddr),
            reinterpret_cast<LPVOID>(Hooking::DetourGameThread),
            reinterpret_cast<LPVOID*>(&Hooking::pOriginalGameThreadUpdate)
        )) {
            LOG_ERROR("[Hooks] Failed to create GameThread hook.");
            return false;
        }

        if (!Hooking::HookManager::EnableHook(reinterpret_cast<LPVOID>(gameThreadFuncAddr))) {
            LOG_ERROR("[Hooks] Failed to enable GameThread hook.");
            return false;
        }

        s_initialized = true;
        LOG_INFO("[Hooks] GameThread hook created and enabled.");
        return true;
    }

    void CleanupHooks() {
        LOG_INFO("[Hooks] Cleaning up...");

        uintptr_t gameThreadFuncAddr = AddressManager::GetGameThreadUpdateFunc();
        if (gameThreadFuncAddr && Hooking::pOriginalGameThreadUpdate) {
            Hooking::HookManager::DisableHook(reinterpret_cast<LPVOID>(gameThreadFuncAddr));
            Hooking::HookManager::RemoveHook(reinterpret_cast<LPVOID>(gameThreadFuncAddr));
            LOG_INFO("[Hooks] GameThread hook cleaned up.");
        }

#ifndef GW2AL_BUILD
        // D3DRenderHook::Shutdown() now handles Present hook cleanup internally
        Hooking::D3DRenderHook::Shutdown();
#endif

        Hooking::HookManager::Shutdown();

        LOG_INFO("[Hooks] Cleanup finished.");
    }

} // namespace kx
