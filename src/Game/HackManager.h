#pragma once

#include <glm/vec3.hpp>

namespace kx {

	class TeleportManager {
		public:
		 // Teleport local player to a target position expressed in Mumble meters (Y-up)
		 // Returns true on success.
		 static bool TeleportToMumblePosition(const glm::vec3& targetMumblePos);
	};

} // namespace kx
