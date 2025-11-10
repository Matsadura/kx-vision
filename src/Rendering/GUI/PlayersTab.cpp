#include "PlayersTab.h"
#include "GuiHelpers.h"
#include "../Core/ESPRenderer.h"
#include "../Data/ESPEntityTypes.h"
#include "../Utils/ESPFormatting.h"
#include "../../../libs/ImGui/imgui.h"
#include "../../Core/AppState.h"
#include "../../Game/HackManager.h"
#include <cstdio>
#include <string>
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

            void RenderVisiblePlayersTable() {
                auto& settings = AppState::Get().GetSettings();
                const auto& visualsData = ESPRenderer::GetProcessedRenderData();
                const auto& filteredData = ESPRenderer::GetFilteredRenderData();
                bool includeOffscreen = settings.playerESP.listOffscreenEntities;
                if (ImGui::CollapsingHeader("Visible Players", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Checkbox("List Off-Screen##Players", &settings.playerESP.listOffscreenEntities);

                    // Search bar (persistent for session)
                    static char playerSearch[64] = "";
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(200.0f);
                    ImGui::InputTextWithHint("##PlayerSearch", "Search players...", playerSearch, IM_ARRAYSIZE(playerSearch));
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Clear##PlayerSearch")) { playerSearch[0] = '\0'; }

                    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchSame;
                    ImVec2 tableSize(0,200); // fixed height with vertical scrolling
                    if (ImGui::BeginTable("VisiblePlayersTable",1, flags, tableSize)) {
                        if (includeOffscreen) {
                            // Use filtered raw list (pre screen cull) so off-screen entities appear
                            for (const auto* player : filteredData.players) {
                                if (!player) continue;
                                std::string name = !player->playerName.empty() ? player->playerName : player->characterName;
                                if (name.empty()) {
                                    if (const char* prof = ESPFormatting::GetProfessionName(player->profession)) name = prof;
                                    else name = "(Unknown)";
                                }
                                if (!CaseInsensitiveFind(name, playerSearch)) continue; // search filter
                                float dist = player->gameplayDistance;
                                char buf[256];
                                std::snprintf(buf, sizeof(buf), "%s - %.1fm", name.c_str(), dist);

                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                if (ImGui::Selectable(buf, false, ImGuiSelectableFlags_SpanAllColumns)) {
                                    // Teleport to this player's current world position (already in Mumble meters)
                                    TeleportManager::TeleportToMumblePosition(player->position);
                                }
                            }
                        } else {
                            // Use processed (on-screen) finalized entities
                            for (const auto& item : visualsData.finalizedEntities) {
                                if (item.context.entityType != ESPEntityType::Player) continue;
                                std::string name = item.context.playerName;
                                const auto* player = static_cast<const RenderablePlayer*>(item.entity);
                                if (name.empty() && player) {
                                    if (const char* prof = ESPFormatting::GetProfessionName(player->profession)) name = prof;
                                }
                                if (name.empty()) name = "(Unknown)";
                                if (!CaseInsensitiveFind(name, playerSearch)) continue; // search filter
                                float dist = item.entity->gameplayDistance;
                                char buf[256];
                                std::snprintf(buf, sizeof(buf), "%s - %.1fm", name.c_str(), dist);

                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                if (ImGui::Selectable(buf, false, ImGuiSelectableFlags_SpanAllColumns)) {
                                    TeleportManager::TeleportToMumblePosition(item.entity->position);
                                }
                            }
                        }
                        ImGui::EndTable();
                    }
                }
            }

        } // anonymous namespace

        void RenderPlayersTab() {
            if (ImGui::BeginTabItem("Players")) {
                auto& settings = AppState::Get().GetSettings();

                ImGui::Checkbox("Enable Player ESP", &settings.playerESP.enabled);

                if (settings.playerESP.enabled) {
                    ImGui::Separator();

                    if (ImGui::CollapsingHeader("Attitude Filter")) {
                        ImGui::Checkbox("Show Friendly", &settings.playerESP.showFriendly);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Hostile", &settings.playerESP.showHostile);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Neutral", &settings.playerESP.showNeutral);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Indifferent", &settings.playerESP.showIndifferent);
                    }

                    if (ImGui::CollapsingHeader("Combat Emphasis")) {
                        ImGui::PushItemWidth(250.0f);
                        ImGui::SliderFloat("Hostile Player Boost", &settings.playerESP.hostileBoostMultiplier,1.0f,3.0f, "%.1fx");
                        ImGui::PopItemWidth();
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Size multiplier for hostile player text and health bars.\n1.0x: No boost\n2.0x: Default\n3.0x: Maximum emphasis");
                        }
                    }

                    if (ImGui::CollapsingHeader("Player Filter Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Checkbox("Show Local Player", &settings.playerESP.showLocalPlayer);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show your own character in the ESP overlay.");

                        const char* energyTypes[] = { "Dodge", "Special/Mount" };
                        int energyTypeInt = static_cast<int>(settings.playerESP.energyDisplayType);
                        ImGui::PushItemWidth(250.0f);
                        if (ImGui::Combo("Energy Bar Source", &energyTypeInt, energyTypes, IM_ARRAYSIZE(energyTypes))) {
                            settings.playerESP.energyDisplayType = static_cast<EnergyDisplayType>(energyTypeInt);
                        }
                        ImGui::PopItemWidth();

                        const char* gearModes[] = { "Off", "Compact (Top3 Stat Sets)", "Compact (Top3 Attributes)", "Detailed" };
                        ImGui::PushItemWidth(250.0f);
                        int gearModeInt = static_cast<int>(settings.playerESP.gearDisplayMode);
                        if (ImGui::Combo("Gear Display", &gearModeInt, gearModes, IM_ARRAYSIZE(gearModes))) {
                            settings.playerESP.gearDisplayMode = static_cast<GearDisplayMode>(gearModeInt);
                        }
                        ImGui::PopItemWidth();
                    }

                    ImGui::Separator();

                    if (ImGui::CollapsingHeader("Visual Style", ImGuiTreeNodeFlags_DefaultOpen)) {
                        RenderPlayerStyleSettings(settings.playerESP);
                    }

                    if (ImGui::CollapsingHeader("Detailed Information")) {
                        ImGui::Checkbox("Show Details Panel", &settings.playerESP.renderDetails);
                        if (settings.playerESP.renderDetails) {
                            ImGui::Indent();
                            ImGui::Checkbox("Level##PlayerDetail", &settings.playerESP.showDetailLevel);
                            ImGui::SameLine();
                            ImGui::Checkbox("Profession##PlayerDetail", &settings.playerESP.showDetailProfession);
                            ImGui::SameLine();
                            ImGui::Checkbox("Attitude##PlayerDetail", &settings.playerESP.showDetailAttitude);
                            ImGui::SameLine();
                            ImGui::Checkbox("Race##PlayerDetail", &settings.playerESP.showDetailRace);
                            ImGui::Checkbox("HP##PlayerDetail", &settings.playerESP.showDetailHp);
                            ImGui::SameLine();
                            ImGui::Checkbox("Energy##PlayerDetail", &settings.playerESP.showDetailEnergy);
                            ImGui::SameLine();
                            ImGui::Checkbox("Position##PlayerDetail", &settings.playerESP.showDetailPosition);
                            ImGui::Unindent();
                        }
                    }

                    if (ImGui::CollapsingHeader("Movement Trails")) {
                        ImGui::Checkbox("Enable Trails", &settings.playerESP.trails.enabled);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show smooth movement trails behind players.");

                        if (settings.playerESP.trails.enabled) {
                            ImGui::Indent();

                            const char* displayModes[] = { "Hostile Only", "All Players" };
                            int displayModeInt = static_cast<int>(settings.playerESP.trails.displayMode);
                            ImGui::PushItemWidth(250.0f);
                            if (ImGui::Combo("Display Mode", &displayModeInt, displayModes, IM_ARRAYSIZE(displayModes))) {
                                settings.playerESP.trails.displayMode = static_cast<TrailDisplayMode>(displayModeInt);
                            }
                            ImGui::PopItemWidth();

                            const char* teleportModes[] = { "Tactical (Break)", "Analysis (Dotted)" };
                            int teleportModeInt = static_cast<int>(settings.playerESP.trails.teleportMode);
                            ImGui::PushItemWidth(250.0f);
                            if (ImGui::Combo("Teleport Behavior", &teleportModeInt, teleportModes, IM_ARRAYSIZE(teleportModes))) {
                                settings.playerESP.trails.teleportMode = static_cast<TrailTeleportMode>(teleportModeInt);
                            }
                            ImGui::PopItemWidth();

                            ImGui::PushItemWidth(250.0f);
                            ImGui::SliderInt("Max Trail Points", &settings.playerESP.trails.maxPoints,15,60);
                            ImGui::PopItemWidth();

                            ImGui::PushItemWidth(250.0f);
                            ImGui::SliderFloat("Max Duration (s)", &settings.playerESP.trails.maxDuration,0.5f,3.0f, "%.1f");
                            ImGui::PopItemWidth();

                            ImGui::PushItemWidth(250.0f);
                            ImGui::SliderFloat("Line Thickness", &settings.playerESP.trails.thickness,1.0f,5.0f, "%.1f");
                            ImGui::PopItemWidth();

                            ImGui::Unindent();
                        }
                    }

                    // New: list of visible players with teleport-on-click
                    RenderVisiblePlayersTable();
                }
                ImGui::EndTabItem();
            }
        }

    } // namespace GUI
} // namespace kx


