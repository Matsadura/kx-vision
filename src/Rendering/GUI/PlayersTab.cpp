#include "PlayersTab.h"
#include "../../../libs/ImGui/imgui.h"
#include "../../Core/AppState.h"

namespace kx {
    namespace GUI {
        void RenderPlayersTab() {
            if (ImGui::BeginTabItem("Players")) {
                auto& settings = kx::AppState::Get().GetSettings();
                
                ImGui::Checkbox("Enable Player ESP", &settings.playerESP.enabled);
                
                if (settings.playerESP.enabled) {
                    ImGui::Separator();
                    ImGui::Text("Player Filter Options");
                    ImGui::Checkbox("Show Local Player", &settings.playerESP.showLocalPlayer);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Show your own character in the ESP overlay.");
                    }

                    ImGui::Checkbox("Show Gear Info", &settings.playerESP.showGearInfo);
                }
                ImGui::EndTabItem();
            }
        }
    }
}
