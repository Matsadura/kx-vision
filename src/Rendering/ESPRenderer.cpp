#define NOMINMAX

#include "ESPRenderer.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <gtc/type_ptr.hpp>

#include "ESP_Helpers.h"
#include "../../libs/ImGui/imgui.h"
#include "../Core/AppState.h"
#include "../Game/AddressManager.h"
#include "../Game/GameStructs.h"
#include "../Game/ReClassStructs.h"

namespace kx {

// Helper to convert wide-character string to UTF-8 string
std::string WStringToString(const wchar_t* wstr) {
    if (!wstr || wstr[0] == L'\0') return "";

    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size_needed == 0) return "";

    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);

    strTo.pop_back(); // Remove the null terminator from the string's size
    return strTo;
}

// Helper to convert profession ID to string
std::string ProfessionToString(uint32_t profId) {
    switch (profId) {
        case 1: return "Guardian";
        case 2: return "Warrior";
        case 3: return "Engineer";
        case 4: return "Ranger";
        case 5: return "Thief";
        case 6: return "Elementalist";
        case 7: return "Mesmer";
        case 8: return "Necromancer";
        case 9: return "Revenant";
        default: return "Prof ID: " + std::to_string(profId);
    }
}

// Helper to convert race ID to string
std::string RaceToString(uint8_t raceId) {
    switch (raceId) {
        case 0: return "Asura";
        case 1: return "Charr";
        case 2: return "Human";
        case 3: return "Norn";
        case 4: return "Sylvari";
        default: return "Race ID: " + std::to_string(raceId);
    }
}

// Initialize the static camera pointer
Camera* ESPRenderer::s_camera = nullptr;

void ESPRenderer::Initialize(Camera& camera) {
    s_camera = &camera;
}

void ESPRenderer::RenderEntity(ImDrawList* drawList, const glm::vec3& worldPos, float distance, float screenWidth, float screenHeight, unsigned int color, const std::vector<std::string>& details, float healthPercent) {
    if (g_settings.espUseDistanceLimit && distance > g_settings.espRenderDistanceLimit) {
        return;
    }

    glm::vec2 screenPos;
    if (ESP_Helpers::WorldToScreen(worldPos, *s_camera, screenWidth, screenHeight, screenPos)) {
        float boxSize = std::max(4.0f, 15.0f * (50.0f / (distance + 20.0f)));
        float boxTop = screenPos.y - boxSize / 2;
        float boxBottom = screenPos.y + boxSize / 2;
        float boxLeft = screenPos.x - boxSize / 2;
        float boxRight = screenPos.x + boxSize / 2;

        // Health bar (if applicable)
        if (healthPercent >= 0.0f) {
            float barHeight = std::max(2.0f, boxSize);
            float barWidth = 3.0f;
            float healthHeight = barHeight * healthPercent;

            drawList->AddRectFilled(ImVec2(boxLeft - barWidth - 2, boxTop), ImVec2(boxLeft - 2, boxBottom), IM_COL32(0, 0, 0, 180));
            drawList->AddRectFilled(ImVec2(boxLeft - barWidth - 2, boxBottom - healthHeight), ImVec2(boxLeft - 2, boxBottom), IM_COL32(0, 200, 0, 255));
            drawList->AddRect(ImVec2(boxLeft - barWidth - 2, boxTop), ImVec2(boxLeft - 2, boxBottom), IM_COL32(0, 0, 0, 255));
        }

        if (g_settings.espRenderBox) {
            drawList->AddRect(ImVec2(boxLeft, boxTop), ImVec2(boxRight, boxBottom), color, 1.0f, ImDrawFlags_RoundCornersAll, 1.5f);
        }

        float textY = boxTop;

        if (g_settings.espRenderDistance) {
            char distText[32];
            snprintf(distText, sizeof(distText), "%.1fm", distance);
            ::ImVec2 textSize = ImGui::CalcTextSize(distText);
            drawList->AddRectFilled(ImVec2(screenPos.x - textSize.x / 2 - 2, textY - textSize.y - 4), ImVec2(screenPos.x + textSize.x / 2 + 2, textY), IM_COL32(0, 0, 0, 180));
            drawList->AddText(ImVec2(screenPos.x - textSize.x / 2, textY - textSize.y - 2), IM_COL32(255, 255, 255, 255), distText);
        }

        if (g_settings.espRenderDot) {
            drawList->AddCircleFilled(::ImVec2(screenPos.x, screenPos.y), 2.0f, IM_COL32(255, 255, 255, 255));
        }

        if (g_settings.espRenderDetails && !details.empty()) {
            float textYDetails = boxBottom + 2;
            for (const auto& detailText : details) {
                if (detailText.empty()) continue;
                ::ImVec2 textSize = ImGui::CalcTextSize(detailText.c_str());
                drawList->AddRectFilled(ImVec2(screenPos.x - textSize.x / 2 - 2, textYDetails), ImVec2(screenPos.x + textSize.x / 2 + 2, textYDetails + textSize.y + 4), IM_COL32(0, 0, 0, 180));
                drawList->AddText(ImVec2(screenPos.x - textSize.x / 2, textYDetails + 2), IM_COL32(255, 255, 255, 255), detailText.c_str());
                textYDetails += textSize.y + 6;
            }
        }
    }
}

void ESPRenderer::Render(float screenWidth, float screenHeight, const MumbleLinkData* mumbleData) {
    if (!s_camera || ShouldHideESP(mumbleData)) {
        return;
    }

    ::ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const float scaleFactor = 1.23f;

    // --- Render Agents ---
    if (g_settings.espRenderAgents) {
        try {
            uintptr_t agentArrayPtr = AddressManager::GetAgentArray();
            if (agentArrayPtr) {
                AgentArray agentArray(reinterpret_cast<void*>(agentArrayPtr));
                uint32_t count = agentArray.Count();
                for (uint32_t i = 0; i < count; ++i) {
                    Agent agent = agentArray.GetAgent(i);
                    if (!agent) continue;
                    Coordinates3D gameWorldPos = agent.GetPosition();
                    if (gameWorldPos.X == 0.0f && gameWorldPos.Y == 0.0f && gameWorldPos.Z == 0.0f) continue;
                    glm::vec3 cameraWorldPos(gameWorldPos.X / scaleFactor, gameWorldPos.Z / scaleFactor, gameWorldPos.Y / scaleFactor);
                    float distance = glm::length(cameraWorldPos - s_camera->GetPlayerPosition());
                    unsigned int color = IM_COL32(std::min(255, int(255 * (1.0f - distance / 200.0f))), 100, std::min(255, int(255 * (distance / 200.0f))), 200);
                    std::vector<std::string> details;
                    if (g_settings.espRenderDetails) {
                        char typeText[64], gadgetText[64];
                        snprintf(typeText, sizeof(typeText), "Type: %d", agent.GetType());
                        snprintf(gadgetText, sizeof(gadgetText), "Gadget: %d", agent.GetGadgetType());
                        details.push_back(typeText);
                        details.push_back(gadgetText);
                    }
                    RenderEntity(drawList, cameraWorldPos, distance, screenWidth, screenHeight, color, details);
                }
            }
        }
        catch (...) { /* Prevent crash */ }
    }

    // --- Render Characters ---
    if (g_settings.espRenderCharacters) {
        void* pContextCollection = AddressManager::GetContextCollectionPtr();
        if (pContextCollection) {
            try {
                kx::ReClass::ContextCollection ctxCollection(pContextCollection);
                if (!ctxCollection) return;
                kx::ReClass::ChCliContext charContext = ctxCollection.GetChCliContext();
                if (!charContext) return;

                std::map<void*, const wchar_t*> characterNameToPlayerName;
                kx::ReClass::ChCliPlayer** playerList = charContext.GetPlayerList();
                uint32_t playerCount = charContext.GetPlayerListSize();
                if (playerList && playerCount < 2000) {
                    for (uint32_t i = 0; i < playerCount; ++i) {
                        kx::ReClass::ChCliPlayer player(playerList[i]);
                        if (!player) continue;
                        kx::ReClass::ChCliCharacter character = player.GetCharacter();
                        const wchar_t* name = player.GetName();
                        if (character.data() && name) {
                            characterNameToPlayerName[character.data()] = name;
                        }
                    }
                }

                kx::ReClass::ChCliCharacter** characterList = charContext.GetCharacterList();
                uint32_t characterCapacity = charContext.GetCharacterListCapacity();
                if (!characterList || characterCapacity > 8000) return;

                for (uint32_t i = 0; i < characterCapacity; ++i) {
                    kx::ReClass::ChCliCharacter character(characterList[i]);
                    if (!character) continue;

                    kx::ReClass::AgChar agent = character.GetAgent();
                    if (!agent) continue;
                    kx::ReClass::CoChar coChar = agent.GetCoChar();
                    if (!coChar) continue;

                    Coordinates3D gameWorldPos = coChar.GetVisualPosition();
                    if (gameWorldPos.X == 0.0f && gameWorldPos.Y == 0.0f && gameWorldPos.Z == 0.0f) continue;

                    glm::vec3 cameraWorldPos(gameWorldPos.X / scaleFactor, gameWorldPos.Z / scaleFactor, gameWorldPos.Y / scaleFactor);
                    float distance = glm::length(cameraWorldPos - s_camera->GetPlayerPosition());

                    uint32_t attitude = character.GetAttitude();
                    unsigned int color;
                    switch (attitude) {
                        case 0: color = IM_COL32(0, 255, 100, 220); break; // Friendly
                        case 2: color = IM_COL32(255, 50, 50, 220); break;  // Hostile
                        default: color = IM_COL32(255, 255, 100, 220); break;// Neutral
                    }

                    kx::ReClass::ChCliHealth health = character.GetHealth();
                    float healthPercent = -1.0f;
                    if (health) {
                        float maxHealth = health.GetMax();
                        if (maxHealth > 0) healthPercent = health.GetCurrent() / maxHealth;
                    }

                    std::vector<std::string> details;
                    if (g_settings.espRenderDetails) {
                        auto it = characterNameToPlayerName.find(character.data());
                        if (it != characterNameToPlayerName.end()) {
                            details.push_back(WStringToString(it->second));
                        }

                        kx::ReClass::ChCliCoreStats stats = character.GetCoreStats();
                        if (stats) {
                            char levelText[32];
                            snprintf(levelText, sizeof(levelText), "Lvl: %u", stats.GetLevel());
                            std::string prof = ProfessionToString(stats.GetProfession());
                            std::string race = RaceToString(stats.GetRace());
                            details.push_back(std::string(levelText) + " " + race + " " + prof);
                        }

                        kx::ReClass::ChCliEnergies energies = character.GetEnergies();
                        if (energies) {
                            float maxEnergy = energies.GetMax();
                            if (maxEnergy > 0) {
                                char energyText[64];
                                snprintf(energyText, sizeof(energyText), "E: %.0f/%.0f", energies.GetCurrent(), maxEnergy);
                                details.push_back(energyText);
                            }
                        }
                    }

                    RenderEntity(drawList, cameraWorldPos, distance, screenWidth, screenHeight, color, details, healthPercent);
                }
            }
            catch (...) { /* Prevent crash */ }
        }
    }
}

bool ESPRenderer::ShouldHideESP(const MumbleLinkData* mumbleData) {
    if (mumbleData && (mumbleData->context.uiState & IsMapOpen)) {
        return true;
    }
    return false;
}

} // namespace kx
