#pragma once

#include "../../Utils/DebugLogger.h"
#include "../../Utils/SafeForeignClass.h"
#include "../GameEnums.h"
#include "../offsets.h"
#include <glm.hpp>

namespace kx {
    namespace ReClass {

        /**
         * @brief Coordinate/Object wrapper for keyframed entities (gadgets)
         */
        class CoKeyFramed : public kx::SafeForeignClass {
        public:
            CoKeyFramed(void* ptr) : kx::SafeForeignClass(ptr) {}

            glm::vec3 GetPosition() {
                LOG_MEMORY("CoKeyFramed", "GetPosition", data(), Offsets::CO_KEYFRAMED_POSITION);
                
                glm::vec3 position = ReadMember<glm::vec3>(Offsets::CO_KEYFRAMED_POSITION, glm::vec3{0.0f, 0.0f, 0.0f});
                
                LOG_DEBUG("CoKeyFramed::GetPosition - Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
                return position;
            }
        };

        /**
         * @brief Agent wrapper for keyframed entities
         */
        class AgKeyFramed : public kx::SafeForeignClass {
        public:
            AgKeyFramed(void* ptr) : kx::SafeForeignClass(ptr) {}

            CoKeyFramed GetCoKeyFramed() {
                LOG_MEMORY("AgKeyFramed", "GetCoKeyFramed", data(), Offsets::AG_KEYFRAMED_CO_KEYFRAMED);
                
                CoKeyFramed result = ReadPointer<CoKeyFramed>(Offsets::AG_KEYFRAMED_CO_KEYFRAMED);
                
                LOG_PTR("CoKeyFramed", result.data());
                return result;
            }
        };

        /**
         * @brief Client gadget wrapper
         */
        class GdCliGadget : public kx::SafeForeignClass {
        public:
            GdCliGadget(void* ptr) : kx::SafeForeignClass(ptr) {}

            Game::GadgetType GetGadgetType() const {
                LOG_MEMORY("GdCliGadget", "GetGadgetType", data(), Offsets::GD_CLI_GADGET_TYPE);
                
                uint32_t typeValue = ReadMember<uint32_t>(Offsets::GD_CLI_GADGET_TYPE, 0);
                Game::GadgetType gadgetType = static_cast<Game::GadgetType>(typeValue);
                
                LOG_DEBUG("GdCliGadget::GetGadgetType - Type: %u", static_cast<uint32_t>(gadgetType));
                return gadgetType;
            }

            Game::ResourceNodeType GetResourceNodeType() const {
                LOG_MEMORY("GdCliGadget", "GetResourceNodeType", data(), Offsets::GD_CLI_GADGET_RESOURCE_NODE_TYPE);

                return ReadMember<Game::ResourceNodeType>(Offsets::GD_CLI_GADGET_RESOURCE_NODE_TYPE, Game::ResourceNodeType::None);
            }

            bool IsGatherable() {
                LOG_MEMORY("GdCliGadget", "IsGatherable", data(), Offsets::GD_CLI_GADGET_FLAGS);
                
                uint32_t flags = ReadMember<uint32_t>(Offsets::GD_CLI_GADGET_FLAGS, 0);
                bool gatherable = (flags & Offsets::GADGET_FLAG_GATHERABLE) != 0;
                
                LOG_DEBUG("GdCliGadget::IsGatherable - Flags: 0x%X, Gatherable: %s", flags, gatherable ? "true" : "false");
                return gatherable;
            }

            AgKeyFramed GetAgKeyFramed() {
                LOG_MEMORY("GdCliGadget", "GetAgKeyFramed", data(), Offsets::GD_CLI_GADGET_AG_KEYFRAMED);
                
                AgKeyFramed result = ReadPointer<AgKeyFramed>(Offsets::GD_CLI_GADGET_AG_KEYFRAMED);
                
                LOG_PTR("AgKeyFramed", result.data());
                return result;
            }
        };

    } // namespace ReClass
} // namespace kx
