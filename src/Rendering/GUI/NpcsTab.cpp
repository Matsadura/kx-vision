#include "NpcsTab.h"
#include "GuiHelpers.h"
#include "../../../libs/ImGui/imgui.h"
#include "../../Core/AppState.h"

namespace kx {
    namespace GUI {

        void RenderNPCsTab() {
            if (ImGui::BeginTabItem("NPCs")) {
                auto& settings = kx::AppState::Get().GetSettings();
                
                ImGui::Checkbox("Enable NPC ESP", &settings.npcESP.enabled);
                
                if (settings.npcESP.enabled) {
                    ImGui::Separator();
                    if (ImGui::CollapsingHeader("Attitude Filter"))
                    {
                        ImGui::Checkbox("Show Friendly", &settings.npcESP.showFriendly);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Hostile", &settings.npcESP.showHostile);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Neutral", &settings.npcESP.showNeutral);
                        ImGui::Checkbox("Show Indifferent", &settings.npcESP.showIndifferent);
                    }
                    ImGui::Separator();
                    ImGui::Text("Health Filter");
                    ImGui::Checkbox("Show Dead NPCs", &settings.npcESP.showDeadNpcs);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Show NPCs with 0 HP (defeated enemies, corpses).\nUseful for loot opportunities and understanding combat situations.");
                    }

                    ImGui::Separator();
                    if (ImGui::CollapsingHeader("NPC Details Filter")) {
                        ImGui::Checkbox("Level", &settings.npcESP.showDetailLevel);
                        ImGui::SameLine();
                        ImGui::Checkbox("HP", &settings.npcESP.showDetailHp);
                        ImGui::SameLine();
                        ImGui::Checkbox("Attitude", &settings.npcESP.showDetailAttitude);
                        ImGui::SameLine();
                        ImGui::Checkbox("Rank", &settings.npcESP.showDetailRank);
                        ImGui::SameLine();
                        ImGui::Checkbox("Pos", &settings.npcESP.showDetailPosition);
                    }

                    ImGui::Separator();
                    RenderCategoryStyleSettings("NPC Style", settings.npcESP.renderBox, settings.npcESP.renderDistance, settings.npcESP.renderDot, &settings.npcESP.renderHealthBar, nullptr, &settings.npcESP.renderDetails);
                }
                ImGui::EndTabItem();
            }
        }
    }
}
