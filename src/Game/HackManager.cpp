#include "HackManager.h"
#include "AddressManager.h"
#include "ReClass/CharacterStructs.h"
#include "ReClass/AgentStructs.h"
#include "ReClass/ContextStructs.h"
#include "../Rendering/Utils/LayoutConstants.h" // CoordinateTransform
#include "../Utils/DebugLogger.h"
#include "../Utils/MemorySafety.h"

#include <Windows.h>

namespace kx {

	namespace {
		 // Convert from Mumble meters (Y-up) to game visual position (X, Y, Z) using evidence-backed mapping
		 // game.x = mumble.x *1.23
		 // game.y = mumble.z *1.23
		 // game.z = mumble.y *1.23
		 inline glm::vec3 MumbleToGame(const glm::vec3& m) {
			 const float s = CoordinateTransform::GAME_TO_MUMBLE_SCALE_FACTOR; //1.23
			 return glm::vec3(m.x * s, m.z * s, m.y * s);
		 }

		 // Safely write a glm::vec3 to a target address, with protection changes
		 bool SafeWriteVec3(uintptr_t address, const glm::vec3& v) {
			 if (address ==0 || !SafeAccess::IsMemorySafe(reinterpret_cast<void*>(address), sizeof(glm::vec3))) {
				return false;
				}
			 DWORD oldProt =0;
			 if (!VirtualProtect(reinterpret_cast<void*>(address), sizeof(glm::vec3), PAGE_READWRITE, &oldProt)) {
				return false;
			 }
			 bool ok = false;
			 __try {
				 *reinterpret_cast<glm::vec3*>(address) = v;
				 ok = true;
			 } __except (EXCEPTION_EXECUTE_HANDLER) {
				ok = false;
			 }
			 DWORD tmp;
			 VirtualProtect(reinterpret_cast<void*>(address), sizeof(glm::vec3), oldProt, &tmp);
			 return ok;
		 }
	}

	bool TeleportManager::TeleportToMumblePosition(const glm::vec3& targetMumblePos) {
	// Resolve candidate addresses to write
		 auto addressesOpt = AddressManager::ResolvePlayerPositionAddresses();
		 if (!addressesOpt) {
			 LOG_ERROR("[Teleport] Could not resolve local player position addresses");
			 return false;
		 }

		 const auto& addrs = *addressesOpt;
		 // Convert to game visual coordinates
		 const glm::vec3 gamePos = MumbleToGame(targetMumblePos);

		 // Prefer writing to visual position; fall back to secondary/tertiary/physics/grounded if available
		 int successCount =0;
		 if (addrs.visual) {
			successCount += SafeWriteVec3(addrs.visual, gamePos) ?1 :0;
		 }
		 if (addrs.secondary) {
			successCount += SafeWriteVec3(addrs.secondary, gamePos) ?1 :0;
		 }
		 if (addrs.tertiary) {
			successCount += SafeWriteVec3(addrs.tertiary, gamePos) ?1 :0;
		 }
		 if (addrs.physics) {
			successCount += SafeWriteVec3(addrs.physics, gamePos) ?1 :0;
		 }
		 if (addrs.grounded) {
			 // Grounded uses same orientation and unit system as visual position after conversion here
			 successCount += SafeWriteVec3(addrs.grounded, gamePos) ?1 :0;
		 }

		 if (successCount >0) {
			 LOG_INFO("[Teleport] Teleported to (mumble): (%.2f, %.2f, %.2f) -> (game): (%.2f, %.2f, %.2f). Writes=%d",
			 targetMumblePos.x, targetMumblePos.y, targetMumblePos.z,
			 gamePos.x, gamePos.y, gamePos.z, successCount);
			 return true;
		 }

		 LOG_ERROR("[Teleport] Failed to write any position addresses");
		 return false;
	}

} // namespace kx
