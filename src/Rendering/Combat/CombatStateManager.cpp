#include "CombatStateManager.h"
#include "../Utils/ESPConstants.h"
#include <Windows.h>
#include <algorithm>

namespace kx {
	void CombatStateManager::Update(const std::vector<RenderableEntity*>& entities, uint64_t now)
	{
        for (const auto* entity : entities) {
            if (!entity || !entity->isValid || entity->maxHealth <= 0.0f) {
                continue;
            }

            const void* entityId = entity->address;
            float currentHealth = entity->currentHealth;

            // Find or create the state for this entity. The [] operator does this automatically.
            auto& state = m_entityStates[entityId];

            // --- ADAPTIVE DAMAGE ACCUMULATOR FLUSH LOGIC ---
            if (state.accumulatedDamage > 0) {
                bool shouldFlush = false;

                // Condition 1: Has the damage accumulated reached our minimum visual size?
                // Only check if entity has max health to prevent division by zero
                if (entity->maxHealth > 0) {
                    float accumulatedPercent = state.accumulatedDamage / entity->maxHealth;
                    if (accumulatedPercent >= CombatEffects::MIN_VISUAL_CHUNK_PERCENT) {
                        shouldFlush = true; // Flush because the chunk is satisfyingly large.
                    }
                }
                // Condition 2 (Fallback): Has it been too long since the last flush?
                if (!shouldFlush && (now - state.lastFlushTimestamp > CombatEffects::MAX_FLUSH_INTERVAL_MS)) {
                    shouldFlush = true; // Flush because we need to stay responsive.
                }

                if (shouldFlush) {
                    state.accumulatedDamage = 0.0f;
                    // The timer for the *next* flush is started by the next hit, so we don't need to set it here.
                }
            }

            // Only process changes if we have a history for this entity.
            if (state.lastSeenTimestamp > 0) {
                // Check for a health change
                if (currentHealth < state.lastKnownHealth) {
                    // --- DAMAGE & DEATH LOGIC ---
                    float damageTakenThisFrame = state.lastKnownHealth - currentHealth;

                    // *** THE CRITICAL FIX IS HERE ***
                    // If this is the FIRST hit of a new burst (accumulator was empty), start the flush timer.
                    if (state.accumulatedDamage == 0.0f) {
                        state.lastFlushTimestamp = now;
                    }

                    // Now, add the damage to the accumulator.
                    state.accumulatedDamage += damageTakenThisFrame;

                    state.lastDamageTaken = damageTakenThisFrame;
                    state.lastHitTimestamp = now; // Keep this for the "Hit Sparkle"

                    if (currentHealth <= 0.0f) {
                        state.deathTimestamp = now;
                    }
                }
                else if (currentHealth > state.lastKnownHealth) {
                    // --- HEALING & RESPAWN LOGIC ---

                    // This is the CRITICAL FIX: Check if the last known health was zero.
                    // This is the only reliable way to detect a respawn or resurrection.
                    if (state.lastKnownHealth <= 0.0f) {
                        // RESPAWN/REUSE DETECTED.
                        // This is not a heal. Reset the entire state to start fresh.
                        state = {};
                    }
                    else {
                        // This is a genuine heal on a living entity.
                        if (now - state.lastHealTimestamp > CombatEffects::BURST_HEAL_WINDOW_MS) {
                            state.healStartHealth = state.lastKnownHealth;
                        }
                        state.lastHealTimestamp = now;
                        state.lastHealFlashTimestamp = now;
                    }
                }
            }

            // Always update the state for the next frame
            state.lastKnownHealth = currentHealth;
            state.lastSeenTimestamp = now;
        }
	}

    const EntityCombatState* CombatStateManager::GetState(const void* entityId) const {
        auto it = m_entityStates.find(entityId);
        if (it != m_entityStates.end()) {
            return &it->second;
        }
        return nullptr;
    }

    void CombatStateManager::Cleanup() {
        uint64_t now = GetTickCount64();

        for (auto it = m_entityStates.begin(); it != m_entityStates.end(); ) {
            if (now - it->second.lastSeenTimestamp > CombatEffects::STATE_CLEANUP_THRESHOLD_MS) {
                it = m_entityStates.erase(it);
            } else {
                ++it;
            }
        }
    }
}
