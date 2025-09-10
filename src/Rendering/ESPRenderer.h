#pragma once

#include <string>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#include "../Game/Camera.h" // Include Camera header
#include "../Game/MumbleLink.h" // Include MumbleLink for the parameter

// Forward declarations for external types
struct ImDrawList; // Forward declare ImGui's ImDrawList (not in kx namespace)

namespace kx {

class Agent; // Forward declare Agent class in kx namespace

class ESPRenderer {
public:
    static void Initialize(Camera& camera);
    static void Render(float screenWidth, float screenHeight, const MumbleLinkData* mumbleData);

private:
    static void RenderAgents(ImDrawList* drawList, float screenWidth, float screenHeight);
    static void RenderCharacters(ImDrawList* drawList, float screenWidth, float screenHeight);
    static void RenderEntity(ImDrawList* drawList, const glm::vec3& worldPos, float distance, float screenWidth, float screenHeight, unsigned int color, const std::vector<std::string>& details, float healthPercent = -1.0f);
    static bool ShouldHideESP(const MumbleLinkData* mumbleData);

    static Camera* s_camera; // Camera reference for world-to-screen projections
};

} // namespace kx