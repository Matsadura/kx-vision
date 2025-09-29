#pragma once
#include <cstdint>

namespace Offsets {
    // WvContext Offsets
    constexpr uintptr_t WV_CONTEXT_STATUS = 0x58;
    constexpr uintptr_t WV_CONTEXT_PTR_TO_RENDERER = 0x78;

    // Agent Pointer Chain
    constexpr uintptr_t AGENT_PTR_CHAIN_1 = 0xC8;
    constexpr uintptr_t AGENT_PTR_CHAIN_2 = 0x38;

    // AgentBase Members
    constexpr uintptr_t AGENT_BASE_TYPE = 0x8;
    constexpr uintptr_t AGENT_BASE_ID = 0xC;
    constexpr uintptr_t AGENT_BASE_GADGET_TYPE = 0x40;
    constexpr uintptr_t AGENT_BASE_TRANSFORM = 0x50;

    // AgentTransform Members
    constexpr uintptr_t AGENT_TRANSFORM_X = 0x30;
    constexpr uintptr_t AGENT_TRANSFORM_Y = 0x34;
    constexpr uintptr_t AGENT_TRANSFORM_Z = 0x38;

    // AgentArray Members
    constexpr uintptr_t AGENT_ARRAY_POINTER = 0x0;
    constexpr uintptr_t AGENT_ARRAY_CAPACITY = 0x8;
    constexpr uintptr_t AGENT_ARRAY_COUNT = 0xC;

    // Character Health System
    constexpr uintptr_t CH_CLI_HEALTH_CURRENT = 0x0C;
    constexpr uintptr_t CH_CLI_HEALTH_MAX = 0x10;

    // Character Energy System  
    constexpr uintptr_t CH_CLI_ENERGIES_CURRENT = 0x0C;
    constexpr uintptr_t CH_CLI_ENERGIES_MAX = 0x10;

    // Character Core Stats
    constexpr uintptr_t CH_CLI_CORE_STATS_RACE = 0x33;
    constexpr uintptr_t CH_CLI_CORE_STATS_LEVEL = 0xAC;
    constexpr uintptr_t CH_CLI_CORE_STATS_PROFESSION = 0x12C;
    constexpr uintptr_t CH_CLI_CORE_STATS_SCALED_LEVEL = 0x234;

    // Character Main Structure
    constexpr uintptr_t CH_CLI_CHARACTER_AGENT = 0x98;
    constexpr uintptr_t CH_CLI_CHARACTER_HEALTH = 0x03E8;
    constexpr uintptr_t CH_CLI_CHARACTER_ENERGIES = 0x03D8;
    constexpr uintptr_t CH_CLI_CHARACTER_CORE_STATS = 0x0388;
    constexpr uintptr_t CH_CLI_CHARACTER_ATTITUDE = 0x00C0;
    constexpr uintptr_t CH_CLI_CHARACTER_RANK_FLAGS = 0x0264;

    // Character Wrapper (AgChar)
    constexpr uintptr_t AG_CHAR_CO_CHAR = 0x50;
    constexpr uintptr_t AG_CHAR_TYPE = 0x08;

    // Coordinate System (CoChar)
    constexpr uintptr_t CO_CHAR_VISUAL_POSITION = 0x30;

    // Player Wrapper (ChCliPlayer)
    constexpr uintptr_t CH_CLI_PLAYER_CHARACTER_PTR = 0x18;
    constexpr uintptr_t CH_CLI_PLAYER_NAME_PTR = 0x68;

    // Context System - Character Context
    constexpr uintptr_t CH_CLI_CONTEXT_CHARACTER_LIST = 0x60;
    constexpr uintptr_t CH_CLI_CONTEXT_CHARACTER_LIST_CAPACITY = 0x68;
    constexpr uintptr_t CH_CLI_CONTEXT_PLAYER_LIST = 0x80;
    constexpr uintptr_t CH_CLI_CONTEXT_PLAYER_LIST_SIZE = 0x88;
    constexpr uintptr_t CH_CLI_CONTEXT_LOCAL_PLAYER = 0x98;

    // Context System - Gadget Context
    constexpr uintptr_t GD_CLI_CONTEXT_GADGET_LIST = 0x0030;
    constexpr uintptr_t GD_CLI_CONTEXT_GADGET_LIST_CAPACITY = 0x0038;
    constexpr uintptr_t GD_CLI_CONTEXT_GADGET_LIST_COUNT = 0x003C;

    // Context Collection
    constexpr uintptr_t CONTEXT_COLLECTION_CH_CLI_CONTEXT = 0x98;
    constexpr uintptr_t CONTEXT_COLLECTION_GD_CLI_CONTEXT = 0x0138;

    // Gadget System - Position and Type
    constexpr uintptr_t CO_KEYFRAMED_POSITION = 0x0030;
    constexpr uintptr_t AG_KEYFRAMED_CO_KEYFRAMED = 0x0050;
    constexpr uintptr_t GD_CLI_GADGET_TYPE = 0x0200;
    constexpr uintptr_t GD_CLI_GADGET_RESOURCE_NODE_TYPE = 0x04E4;
    constexpr uintptr_t GD_CLI_GADGET_FLAGS = 0x04E8;
    constexpr uintptr_t GD_CLI_GADGET_AG_KEYFRAMED = 0x0038;

    // Gadget Flags
    constexpr uint32_t GADGET_FLAG_GATHERABLE = 0x2;

    // Equipment System
    constexpr uintptr_t CH_CLI_CHARACTER_INVENTORY = 0x3F0;  // Ptr to Inventory struct from ChCliCharacter
    constexpr uintptr_t INVENTORY_EQUIPMENT_ARRAY = 0x160;  // Ptr to array of EquipSlot pointers from Inventory
    constexpr uintptr_t EQUIP_SLOT_ITEM_DEF = 0x40;         // Ptr to ItemDef from EquipSlot
    constexpr uintptr_t EQUIP_SLOT_STAT_GEAR = 0xA0;        // Ptr to Stat for Armor/Trinkets from EquipSlot
    constexpr uintptr_t EQUIP_SLOT_STAT_WEAPON = 0xA8;        // Ptr to Stat for Weapons from EquipSlot
    constexpr uintptr_t ITEM_DEF_ID = 0x28;                 // Item's ID from ItemDef
    constexpr uintptr_t ITEM_DEF_RARITY = 0x60;
    constexpr uintptr_t STAT_ID = 0x28;                     // Stat combination ID from Stat

    // --- Historical/Unverified Offsets from old GearCheck ---
    // These are good starting points for finding runes, sigils, and rarity.
    // They require verification before use.
    // constexpr uintptr_t EQUIP_SLOT_RUNE = 0xC0;
    // constexpr uintptr_t EQUIP_SLOT_SIGIL1 = 0xC8;
    // constexpr uintptr_t EQUIP_SLOT_SIGIL2 = 0xD0;
}
