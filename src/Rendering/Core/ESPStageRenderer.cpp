#include "ESPStageRenderer.h"
#include "../Renderers/ESPContextFactory.h"
#include "../../Core/AppState.h"
#include "../../Game/Camera.h"
#include "../Utils/ESPMath.h"
#include "../Utils/ESPConstants.h"
#include "../Utils/ESPPlayerDetailsBuilder.h"
#include "../Utils/ESPEntityDetailsBuilder.h"
#include "../Utils/ESPStyling.h"
#include "../Renderers/ESPShapeRenderer.h"
#include "../Renderers/ESPTextRenderer.h"
#include "../Renderers/ESPHealthBarRenderer.h"
#include "../Data/EntityRenderContext.h"
#include "../Utils/ESPFormatting.h"
#include "../../../libs/ImGui/imgui.h"
#include <sstream>
#include <iomanip>
#include "../Text/TextElementFactory.h"
#include "../Utils/EntityVisualsCalculator.h"
#include "Text/TextRenderer.h"
#include <optional>

namespace kx {

// Helper function to build the render context.
// This consolidates the logic from the old RenderPooledPlayers/Npcs/Gadgets functions.
static EntityRenderContext CreateEntityRenderContextForRendering(const RenderableEntity* entity, const FrameContext& context) {
    std::vector<ColoredDetail> details;
    // Use a switch on entity->entityType to call the correct details builder
    switch(entity->entityType) {
        case ESPEntityType::Player:
        {
            const auto* player = static_cast<const RenderablePlayer*>(entity);
            details = ESPPlayerDetailsBuilder::BuildPlayerDetails(player, context.settings.playerESP, context.settings.showDebugAddresses);
            if (context.settings.playerESP.gearDisplayMode == GearDisplayMode::Detailed) {
                auto gearDetails = ESPPlayerDetailsBuilder::BuildGearDetails(player);
                if (!gearDetails.empty()) {
                    if (!details.empty()) {
                        details.push_back({ "--- Gear Stats ---", ESPColors::DEFAULT_TEXT });
                    }
                    details.insert(details.end(), gearDetails.begin(), gearDetails.end());
                }
            }
            break;
        }
        case ESPEntityType::NPC:
        {
            const auto* npc = static_cast<const RenderableNpc*>(entity);
            details = ESPEntityDetailsBuilder::BuildNpcDetails(npc, context.settings.npcESP, context.settings.showDebugAddresses);
            break;
        }
        case ESPEntityType::Gadget:
        {
            const auto* gadget = static_cast<const RenderableGadget*>(entity);
            details = ESPEntityDetailsBuilder::BuildGadgetDetails(gadget, context.settings.objectESP, context.settings.showDebugAddresses);
            break;
        }
    }

    // Now, create the context using the ESPContextFactory, just like before.
    // We pass the main 'context' directly.
    switch(entity->entityType) {
        case ESPEntityType::Player:
            return ESPContextFactory::CreateContextForPlayer(static_cast<const RenderablePlayer*>(entity), details, context);
        case ESPEntityType::NPC:
            return ESPContextFactory::CreateContextForNpc(static_cast<const RenderableNpc*>(entity), details, context);
        case ESPEntityType::Gadget:
            return ESPContextFactory::CreateContextForGadget(static_cast<const RenderableGadget*>(entity), details, context);
    }
    // This should not be reached, but we need to return something.
    // Returning a gadget context as a fallback.
    return ESPContextFactory::CreateContextForGadget(static_cast<const RenderableGadget*>(entity), details, context);
}

std::optional<VisualProperties> ESPStageRenderer::CalculateLiveVisuals(const FinalizedRenderable& item, const FrameContext& context) {
    // 1. Re-project the entity's world position to get a fresh screen position.
    glm::vec2 freshScreenPos;
    if (!ESPMath::WorldToScreen(item.entity->position, context.camera, context.screenWidth, context.screenHeight, freshScreenPos)) {
        return std::nullopt; // Cull if off-screen this frame.
    }

    // 2. Make a mutable copy of the cached visual properties.
    VisualProperties liveVisuals = item.visuals;

    // 3. Overwrite the stale screen-space properties with fresh ones.
    liveVisuals.screenPos = freshScreenPos;
    
    // 4. Recalculate derived screen-space coordinates using the cached dimensions.
    if (item.entity->entityType == ESPEntityType::Gadget) {
        liveVisuals.center = ImVec2(liveVisuals.screenPos.x, liveVisuals.screenPos.y);
        liveVisuals.boxMin = ImVec2(liveVisuals.center.x - liveVisuals.circleRadius, liveVisuals.center.y - liveVisuals.circleRadius);
        liveVisuals.boxMax = ImVec2(liveVisuals.center.x + liveVisuals.circleRadius, liveVisuals.center.y + liveVisuals.circleRadius);
    } else {
        float boxWidth = liveVisuals.boxMax.x - liveVisuals.boxMin.x;
        float boxHeight = liveVisuals.boxMax.y - liveVisuals.boxMin.y;
        
        liveVisuals.boxMin = ImVec2(liveVisuals.screenPos.x - boxWidth / 2.0f, liveVisuals.screenPos.y - boxHeight);
        liveVisuals.boxMax = ImVec2(liveVisuals.screenPos.x + boxWidth / 2.0f, liveVisuals.screenPos.y);
        liveVisuals.center = ImVec2(liveVisuals.screenPos.x, liveVisuals.screenPos.y - boxHeight / 2.0f);
    }
    
    return liveVisuals;
}

void ESPStageRenderer::RenderFrameData(const FrameContext& context, const PooledFrameRenderData& frameData) {
    for (const auto& item : frameData.finalizedEntities) {
        
        // First, perform the high-frequency update to get live visual properties for this frame.
        auto liveVisualsOpt = CalculateLiveVisuals(item, context);
        
        // If the entity is on-screen, proceed with rendering.
        if (liveVisualsOpt) {
            // Create the cheap render context.
            EntityRenderContext entityContext = CreateEntityRenderContextForRendering(item.entity, context);
            
            // Render using the fresh visual properties.
            RenderEntityComponents(context, entityContext, *liveVisualsOpt);
        }
    }
}

void ESPStageRenderer::RenderEntityComponents(const FrameContext& context, const EntityRenderContext& entityContext, const VisualProperties& props) {
    // The logic inside this function remains almost identical.
    // It just uses the `props` passed into it instead of calculating them.
    RenderHealthBar(context.drawList, entityContext, props, context.settings);
    RenderEnergyBar(context.drawList, entityContext, props, context.settings);
    RenderBoundingBox(context.drawList, entityContext, props);
    RenderGadgetVisuals(context.drawList, entityContext, context.camera, props, context.settings);
    RenderDistanceText(context.drawList, entityContext, props);
    RenderCenterDot(context.drawList, entityContext, props);
    RenderPlayerName(context.drawList, entityContext, props);
    RenderDetailsText(context.drawList, entityContext, props);
    RenderGearSummary(context.drawList, entityContext, props, context.settings);

    // Render independent text elements last
    RenderDamageNumbers(context, entityContext, props);
    RenderBurstDps(context, entityContext, props);
}

// Component rendering implementations
void ESPStageRenderer::RenderHealthBar(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props, const Settings& settings) {
    bool isLivingEntity = (context.entityType == ESPEntityType::Player || context.entityType == ESPEntityType::NPC);
    bool isGadget = (context.entityType == ESPEntityType::Gadget);

    // Render standalone health bars for living entities when health is available AND setting is enabled
    if ((isLivingEntity || isGadget) && context.healthPercent >= 0.0f && context.renderHealthBar) {
        ESPHealthBarRenderer::RenderStandaloneHealthBar(drawList, props.screenPos, context,
            props.fadedEntityColor, props.finalHealthBarWidth, props.finalHealthBarHeight, props.finalFontSize);
    }
}

void ESPStageRenderer::RenderEnergyBar(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props, const Settings& settings) {
    // Render energy bar for players
    if (context.entityType == ESPEntityType::Player && context.energyPercent >= 0.0f && context.renderEnergyBar) {
        ESPHealthBarRenderer::RenderStandaloneEnergyBar(drawList, props.screenPos, context.energyPercent,
            props.finalAlpha, props.finalHealthBarWidth, props.finalHealthBarHeight,
            props.finalHealthBarHeight);
    }
}

void ESPStageRenderer::RenderBoundingBox(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props) {
    bool isGadget = (context.entityType == ESPEntityType::Gadget);

    // Render bounding box for players/NPCs
    if (!isGadget && context.renderBox) {
        ESPShapeRenderer::RenderBoundingBox(drawList, props.boxMin, props.boxMax, props.fadedEntityColor, props.finalBoxThickness);
    }
}

void ESPStageRenderer::RenderGadgetVisuals(ImDrawList* drawList, const EntityRenderContext& context, Camera& camera, const VisualProperties& props, const Settings& settings) {
    bool isGadget = (context.entityType == ESPEntityType::Gadget);

    // Render gadget visuals (non-exclusive)
    if (isGadget) {
        if (settings.objectESP.renderSphere) {
            ESPShapeRenderer::RenderGadgetSphere(drawList, context, camera, props.screenPos, props.finalAlpha, props.fadedEntityColor, props.scale);
        }
        if (settings.objectESP.renderCircle) {
            drawList->AddCircle(ImVec2(props.screenPos.x, props.screenPos.y), props.circleRadius, props.fadedEntityColor, 0, props.finalBoxThickness);
        }
    }
}

void ESPStageRenderer::RenderDistanceText(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props) {
    bool isGadget = (context.entityType == ESPEntityType::Gadget);

    // Render distance text
    if (context.renderDistance) {
        if (isGadget) {
            // For gadgets, position distance text above the circle
            ImVec2 textAnchor(props.center.x, props.center.y - props.circleRadius);
            ESPTextRenderer::RenderDistanceText(drawList, props.center, textAnchor, context.gameplayDistance,
                props.finalAlpha, props.finalFontSize);
        }
        else {
            // For players/NPCs, use traditional positioning
            ESPTextRenderer::RenderDistanceText(drawList, props.center, props.boxMin, context.gameplayDistance,
                props.finalAlpha, props.finalFontSize);
        }
    }
}

void ESPStageRenderer::RenderCenterDot(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props) {
    bool isGadget = (context.entityType == ESPEntityType::Gadget);

    // Render center dot
    if (context.renderDot) {
        if (isGadget) {
            ESPShapeRenderer::RenderNaturalWhiteDot(drawList, props.screenPos, props.finalAlpha, props.finalDotRadius);
        }
        else {
            ESPShapeRenderer::RenderColoredDot(drawList, props.screenPos, props.fadedEntityColor, props.finalDotRadius);
        }
    }
}

void ESPStageRenderer::RenderPlayerName(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props) {
    // Render player name for natural identification (players only)
    if (context.entityType == ESPEntityType::Player && context.renderPlayerName) {
        // For hostile players with an empty name, display their profession
        std::string displayName = context.playerName;
        if (displayName.empty() && context.attitude == Game::Attitude::Hostile) {
            if (context.player) {
                const char* prof = ESPFormatting::GetProfessionName(context.player->profession);
                if (prof) {
                    displayName = prof;
                }
            }
        }

        if (!displayName.empty()) {
            // Use entity color directly (already attitude-based from ESPContextFactory)
            ESPTextRenderer::RenderPlayerName(drawList, props.screenPos, displayName, props.fadedEntityColor, props.finalFontSize);
        }
    }
}

void ESPStageRenderer::RenderDetailsText(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props) {
    bool isGadget = (context.entityType == ESPEntityType::Gadget);

    // Render details text (for all entities when enabled)
    if (context.renderDetails && !context.details.empty()) {
        if (isGadget) {
            // For gadgets, position details below the circle
            ImVec2 textAnchor(props.center.x, props.center.y + props.circleRadius);
            ESPTextRenderer::RenderDetailsText(drawList, props.center, textAnchor, context.details, props.finalAlpha, props.finalFontSize);
        }
        else {
            // For players/NPCs, use traditional positioning
            ESPTextRenderer::RenderDetailsText(drawList, props.center, props.boxMax, context.details, props.finalAlpha, props.finalFontSize);
        }
    }
}

void ESPStageRenderer::RenderGearSummary(ImDrawList* drawList, const EntityRenderContext& context, const VisualProperties& props, const Settings& settings) {
    // Specialized Summary Rendering (Players Only)
    if (context.entityType == ESPEntityType::Player && context.player != nullptr) {
        switch (settings.playerESP.gearDisplayMode) {
        case GearDisplayMode::Compact: { // Compact (Stat Names)
            auto compactSummary = ESPPlayerDetailsBuilder::BuildCompactGearSummary(context.player);
            ESPTextRenderer::RenderGearSummary(drawList, props.screenPos, compactSummary, props.finalAlpha, props.finalFontSize);
            break;
        }
        case GearDisplayMode::Attributes: { // Top 3 Attributes
            auto dominantStats = ESPPlayerDetailsBuilder::BuildDominantStats(context.player);
            auto topRarity = ESPPlayerDetailsBuilder::GetHighestRarity(context.player);
            ESPTextRenderer::RenderDominantStats(drawList, props.screenPos, dominantStats, topRarity, props.finalAlpha, props.finalFontSize);
            break;
        }
        default:
            // Modes Off and Detailed do not have a separate summary view
            break;
        }
    }
}

void ESPStageRenderer::RenderDamageNumbers(const FrameContext& context, const EntityRenderContext& entityContext, const VisualProperties& props) {
    // For gadgets, check if combat UI should be hidden for this type
    if (entityContext.entityType == ESPEntityType::Gadget) {
        const auto* gadget = static_cast<const RenderableGadget*>(entityContext.entity);
        if (gadget && ESPStyling::ShouldHideCombatUIForGadget(gadget->type)) {
            return;
        }
    }

    // Check if the setting is enabled for the specific entity type
    bool isEnabled = false;
    if (entityContext.entityType == ESPEntityType::Player) isEnabled = context.settings.playerESP.showDamageNumbers;
    else if (entityContext.entityType == ESPEntityType::NPC) isEnabled = context.settings.npcESP.showDamageNumbers;
    else if (entityContext.entityType == ESPEntityType::Gadget) isEnabled = context.settings.objectESP.showDamageNumbers;

    if (!isEnabled || entityContext.healthBarAnim.damageNumberAlpha <= 0.0f) {
        return;
    }

    // --- ANCHORING LOGIC ---
    glm::vec2 anchorPos;
    if (entityContext.renderHealthBar) {
        // If HP bar is on, anchor above it for perfect alignment.
        float barY = props.screenPos.y + RenderingLayout::STANDALONE_HEALTH_BAR_Y_OFFSET;
        anchorPos = { props.center.x, barY - entityContext.healthBarAnim.damageNumberYOffset };
    } else {
        // FALLBACK: If HP bar is off, anchor to the entity's visual center.
        anchorPos = { props.center.x, props.center.y - entityContext.healthBarAnim.damageNumberYOffset };
    }

    // --- RENDER LOGIC (moved from ESPHealthBarRenderer) ---
    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << entityContext.healthBarAnim.damageNumberToDisplay;
    float finalFontSize = props.finalFontSize * EntityVisualsCalculator::GetDamageNumberFontSizeMultiplier(entityContext.healthBarAnim.damageNumberToDisplay);
    TextElement element = TextElementFactory::CreateDamageNumber(ss.str(), anchorPos, entityContext.healthBarAnim.damageNumberAlpha, finalFontSize);
    TextRenderer::Render(context.drawList, element);
}

void ESPStageRenderer::RenderBurstDps(const FrameContext& context, const EntityRenderContext& entityContext, const VisualProperties& props) {
    // For gadgets, check if combat UI should be hidden for this type
    if (entityContext.entityType == ESPEntityType::Gadget) {
        const auto* gadget = static_cast<const RenderableGadget*>(entityContext.entity);
        if (gadget && ESPStyling::ShouldHideCombatUIForGadget(gadget->type)) {
            return;
        }
    }

    // Check if the setting is enabled
    bool isEnabled = false;
    if (entityContext.entityType == ESPEntityType::Player) isEnabled = context.settings.playerESP.showBurstDps;
    else if (entityContext.entityType == ESPEntityType::NPC) isEnabled = context.settings.npcESP.showBurstDps;
    else if (entityContext.entityType == ESPEntityType::Gadget) isEnabled = context.settings.objectESP.showBurstDps;

    if (!isEnabled || entityContext.burstDPS <= 0.0f || entityContext.healthBarAnim.healthBarFadeAlpha <= 0.0f) {
        return;
    }
    
    // --- FORMATTING ---
    std::stringstream ss;
    if (entityContext.burstDPS >= 1000.0f) {
        ss << std::fixed << std::setprecision(1) << (entityContext.burstDPS / 1000.0f) << "k";
    } else {
        ss << std::fixed << std::setprecision(0) << entityContext.burstDPS;
    }

    // --- ANCHORING LOGIC ---
    glm::vec2 anchorPos;
    if (entityContext.renderHealthBar) {
        // If HP bar is on, anchor to its right side.
        float barMinY = props.screenPos.y + RenderingLayout::STANDALONE_HEALTH_BAR_Y_OFFSET;
        float barMaxY = barMinY + props.finalHealthBarHeight;
        anchorPos = { props.center.x + props.finalHealthBarWidth * 0.5f + 5.0f, barMinY + (barMaxY - barMinY) * 0.5f };
    } else {
        // FALLBACK: If HP bar is off, anchor below the entity's name/details.
        anchorPos = { props.screenPos.x, props.screenPos.y + 20.0f }; // Adjust Y-offset as needed
    }
    
    // --- RENDER LOGIC ---
    TextElement element(ss.str(), anchorPos, TextAnchor::Custom);
    element.SetAlignment(TextAlignment::Left);
    TextStyle style = TextElementFactory::GetDistanceStyle(entityContext.healthBarAnim.healthBarFadeAlpha, props.finalFontSize * 0.9f);
    style.enableBackground = false;
    style.textColor = IM_COL32(255, 200, 50, 255);
    element.SetStyle(style);
    TextRenderer::Render(context.drawList, element);
}

} // namespace kx
