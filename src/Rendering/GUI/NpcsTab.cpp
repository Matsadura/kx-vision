#include "NpcsTab.h"
#include "GuiHelpers.h"
#include "../Core/ESPRenderer.h"
#include "../Data/ESPEntityTypes.h"
#include "../../../libs/ImGui/imgui.h"
#include "../../Core/AppState.h"
#include <string>
#include <cstdio>
#include <algorithm>
#include <cctype>

namespace kx {
    namespace GUI {

        namespace {

            // Helper: case-insensitive substring match
            static bool CaseInsensitiveFind(const std::string& haystack, const char* needleCStr) {
                if (!needleCStr || !*needleCStr) return true; // empty needle -> always match
                std::string needle(needleCStr);
                std::string h = haystack;
                std::transform(h.begin(), h.end(), h.begin(), [](unsigned char c){ return (char)std::tolower(c); });
                std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c){ return (char)std::tolower(c); });
                return h.find(needle) != std::string::npos;
            }

            void RenderVisibleNpcsTable() {
                auto& settings = AppState::Get().GetSettings();
                const auto& visualsData = ESPRenderer::GetProcessedRenderData();
                const auto& filteredData = ESPRenderer::GetFilteredRenderData();
                bool includeOffscreen = settings.npcESP.listOffscreenEntities;
                if (ImGui::CollapsingHeader("Visible NPCs", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Checkbox("List Off-Screen##NPCs", &settings.npcESP.listOffscreenEntities);

                    // Search bar (persistent for session)
                    static char npcSearch[64] = "";
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(200.0f);
                    ImGui::InputTextWithHint("##NpcSearch", "Search NPCs...", npcSearch, IM_ARRAYSIZE(npcSearch));
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Clear##NpcSearch")) { npcSearch[0] = '\0'; }

                    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchSame;
                    ImVec2 tableSize(0,200);
                    if (ImGui::BeginTable("VisibleNpcsTable",1, flags, tableSize)) {
                        if (includeOffscreen) {
                            for (const auto* npc : filteredData.npcs) {
                                if (!npc) continue;
                                std::string name = npc->name.empty() ? "(NPC)" : npc->name;
                                if (!CaseInsensitiveFind(name, npcSearch)) continue; // filter
                                float dist = npc->gameplayDistance;
                                char buf[256];
                                std::snprintf(buf, sizeof(buf), "%s - %.1fm", name.c_str(), dist);
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(buf);
                            }
                        } else {
                            for (const auto& item : visualsData.finalizedEntities) {
                                if (item.context.entityType != ESPEntityType::NPC) continue;
                                const RenderableNpc* npc = static_cast<const RenderableNpc*>(item.entity);
                                std::string name = npc && !npc->name.empty() ? npc->name : "(NPC)";
                                if (!CaseInsensitiveFind(name, npcSearch)) continue; // filter
                                float dist = item.entity->gameplayDistance;
                                char buf[256];
                                std::snprintf(buf, sizeof(buf), "%s - %.1fm", name.c_str(), dist);
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(buf);
                            }
                        }
                        ImGui::EndTable();
                    }
                }
            }

        } // anonymous namespace

        void RenderNPCsTab() {
            if (ImGui::BeginTabItem("NPCs")) {
                auto& settings = AppState::Get().GetSettings();
                ImGui::Checkbox("Enable NPC ESP", &settings.npcESP.enabled);
                if (settings.npcESP.enabled) {
                    ImGui::Separator();

                    if (ImGui::CollapsingHeader("Attitude Filter")) {
                        ImGui::Checkbox("Show Friendly##NPC", &settings.npcESP.showFriendly);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Hostile##NPC", &settings.npcESP.showHostile);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Neutral##NPC", &settings.npcESP.showNeutral);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Indifferent##NPC", &settings.npcESP.showIndifferent);
                    }

                    if (ImGui::CollapsingHeader("Rank Filter")) {
                        const float column1 =180.0f;
                        const float column2 =360.0f;
                        ImGui::Checkbox("Show Legendary##NPC", &settings.npcESP.showLegendary);
                        ImGui::SameLine(column1);
                        ImGui::Checkbox("Show Champion##NPC", &settings.npcESP.showChampion);
                        ImGui::SameLine(column2);
                        ImGui::Checkbox("Show Elite##NPC", &settings.npcESP.showElite);
                        ImGui::Checkbox("Show Veteran##NPC", &settings.npcESP.showVeteran);
                        ImGui::SameLine(column1);
                        ImGui::Checkbox("Show Ambient##NPC", &settings.npcESP.showAmbient);
                        ImGui::SameLine(column2);
                        ImGui::Checkbox("Show Normal##NPC", &settings.npcESP.showNormal);
                    }

                    if (ImGui::CollapsingHeader("Health Filter")) {
                        ImGui::Checkbox("Show Dead NPCs", &settings.npcESP.showDeadNpcs);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show NPCs with 0 HP (defeated).");
                    }

                    ImGui::Separator();

                    if (ImGui::CollapsingHeader("Visual Style", ImGuiTreeNodeFlags_DefaultOpen)) {
                        RenderNpcStyleSettings(settings.npcESP);
                    }

                    if (ImGui::CollapsingHeader("Detailed Information")) {
                        ImGui::Checkbox("Show Details Panel##NPC", &settings.npcESP.renderDetails);
                        if (settings.npcESP.renderDetails) {
                            ImGui::Indent();
                            ImGui::Checkbox("Level##NpcDetail", &settings.npcESP.showDetailLevel);
                            ImGui::SameLine();
                            ImGui::Checkbox("HP##NpcDetail", &settings.npcESP.showDetailHp);
                            ImGui::SameLine();
                            ImGui::Checkbox("Attitude##NpcDetail", &settings.npcESP.showDetailAttitude);
                            ImGui::SameLine();
                            ImGui::Checkbox("Rank##NpcDetail", &settings.npcESP.showDetailRank);
                            ImGui::SameLine();
                            ImGui::Checkbox("Position##NpcDetail", &settings.npcESP.showDetailPosition);
                            ImGui::Unindent();
                        }
                    }

                    RenderVisibleNpcsTable();
                }
                ImGui::EndTabItem();
            }
        }

    } // namespace GUI
} // namespace kx