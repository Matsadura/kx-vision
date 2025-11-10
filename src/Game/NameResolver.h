#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace kx {
    namespace NameResolver {

        /**
         * @brief Request name resolution for a batch of agent pointers.
         *
         * This must be called from the game thread where the TLS context is valid.
         * The map value represents the agent type (0 = character/NPC, 1 = gadget).
         */
        void CacheNamesForAgents(const std::unordered_map<void*, uint8_t>& agentPointers);

        /**
         * @brief Retrieve a resolved name from the local cache.
         *
         * This call is thread-safe and can be used from any thread.
         * Returns an empty string when no cached value exists.
         */
        std::string GetCachedName(void* agentPtr);

        /**
         * @brief Clear both the resolved name cache and any pending requests.
         */
        void ClearNameCache();

        /**
         * @brief Legacy helper retained for compatibility with earlier code paths.
         */
        std::string GetNameFromAgent(void* agentPtr);

    } // namespace NameResolver
} // namespace kx
