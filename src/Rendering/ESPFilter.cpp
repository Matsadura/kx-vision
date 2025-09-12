#include "ESPFilter.h"
#include "ESPMath.h"
#include <limits>
#include <cmath>

namespace kx {

void ESPFilter::FilterFrameData(const FrameRenderData& rawData, Camera& camera, FrameRenderData& filteredData) {
    // Clear output data
    filteredData.Clear();
    
    // Filter each entity type
    FilterPlayers(rawData.players, camera, filteredData.players);
    FilterNpcs(rawData.npcs, camera, filteredData.npcs);
    FilterGadgets(rawData.gadgets, camera, filteredData.gadgets);
}

void ESPFilter::FilterPlayers(const std::vector<RenderablePlayer>& rawPlayers, Camera& camera, 
                             std::vector<RenderablePlayer>& filteredPlayers) {
    const auto& settings = AppState::Get().GetSettings();
    
    // Early exit if player ESP is disabled
    if (!settings.playerESP.enabled) {
        return;
    }
    
    const glm::vec3 cameraPos = camera.GetPlayerPosition();
    
    // Reserve space to avoid frequent reallocations
    filteredPlayers.reserve(rawPlayers.size());
    
    for (const auto& player : rawPlayers) {
        // Skip invalid entities
        if (!player.isValid) continue;
        
        // Apply local player filter
        if (player.isLocalPlayer && !settings.playerESP.showLocalPlayer) {
            continue;
        }
        
        // Apply health filter (skip dead entities)
        if (!IsHealthValid(player.currentHealth)) {
            continue;
        }
        
        // Apply distance filter
        if (!IsWithinDistanceLimit(player.position, cameraPos, 
                                   settings.espUseDistanceLimit, settings.espRenderDistanceLimit)) {
            continue;
        }
        
        // Calculate distance to camera for filtering
        float distance = glm::length(player.position - cameraPos);
        
        // Entity passed all filters - add to output with pre-calculated distance
        RenderablePlayer filteredPlayer = player;
        filteredPlayer.distance = distance;
        filteredPlayers.push_back(filteredPlayer);
        filteredPlayer.distance = distance;
        filteredPlayers.push_back(filteredPlayer);
    }
}

void ESPFilter::FilterNpcs(const std::vector<RenderableNpc>& rawNpcs, Camera& camera, 
                          std::vector<RenderableNpc>& filteredNpcs) {
    const auto& settings = AppState::Get().GetSettings();
    
    // Early exit if NPC ESP is disabled
    if (!settings.npcESP.enabled) {
        return;
    }
    
    const glm::vec3 cameraPos = camera.GetPlayerPosition();
    
    // Reserve space to avoid frequent reallocations
    filteredNpcs.reserve(rawNpcs.size());
    
    for (const auto& npc : rawNpcs) {
        // Skip invalid entities
        if (!npc.isValid) continue;
        
        // Apply health filter (skip dead entities)
        if (!IsHealthValid(npc.currentHealth)) {
            continue;
        }
        
        // Apply distance filter
        if (!IsWithinDistanceLimit(npc.position, cameraPos, 
                                   settings.espUseDistanceLimit, settings.espRenderDistanceLimit)) {
            continue;
        }
        
        // Apply attitude-based filter using EntityFilter utility (no cast needed - already enum)
        if (!Filtering::EntityFilter::ShouldRenderNpc(npc.attitude, settings.npcESP)) {
            continue;
        }
        
        // Calculate distance to camera for filtering
        float distance = glm::length(npc.position - cameraPos);
        
        // Entity passed all filters - add to output with pre-calculated distance
        RenderableNpc filteredNpc = npc;
        filteredNpc.distance = distance;
        filteredNpcs.push_back(filteredNpc);
    }
}

void ESPFilter::FilterGadgets(const std::vector<RenderableGadget>& rawGadgets, Camera& camera, 
                             std::vector<RenderableGadget>& filteredGadgets) {
    const auto& settings = AppState::Get().GetSettings();
    
    // Early exit if object ESP is disabled
    if (!settings.objectESP.enabled) {
        return;
    }
    
    const glm::vec3 cameraPos = camera.GetPlayerPosition();
    
    // Reserve space to avoid frequent reallocations
    filteredGadgets.reserve(rawGadgets.size());
    
    for (const auto& gadget : rawGadgets) {
        // Skip invalid entities
        if (!gadget.isValid) continue;
        
        // Apply distance filter
        if (!IsWithinDistanceLimit(gadget.position, cameraPos, 
                                   settings.espUseDistanceLimit, settings.espRenderDistanceLimit)) {
            continue;
        }
        
        // Apply gadget type-based filter using EntityFilter utility (no cast needed - already enum)
        if (!Filtering::EntityFilter::ShouldRenderGadget(gadget.type, settings.objectESP)) {
            continue;
        }
        
        // Calculate distance to camera for filtering
        float distance = glm::length(gadget.position - cameraPos);
        
        // Entity passed all filters - add to output with pre-calculated distance
        RenderableGadget filteredGadget = gadget;
        filteredGadget.distance = distance;
        filteredGadgets.push_back(filteredGadget);
        filteredGadget.distance = distance;
        filteredGadgets.push_back(filteredGadget);
    }
}

bool ESPFilter::IsWithinDistanceLimit(const glm::vec3& entityPos, const glm::vec3& cameraPos, 
                                     bool useDistanceLimit, float distanceLimit) {
    if (!useDistanceLimit) {
        return true; // No distance limiting
    }
    
    // Use squared distance comparison to avoid expensive sqrt calculation
    const glm::vec3 deltaPos = entityPos - cameraPos;
    const float distanceSquared = glm::dot(deltaPos, deltaPos);
    const float limitSquared = distanceLimit * distanceLimit;
    
    return distanceSquared <= limitSquared;
}

bool ESPFilter::IsHealthValid(float currentHealth) {
    // Filter out dead entities (0 HP or negative HP)
    return currentHealth > 0.0f;
}

} // namespace kx