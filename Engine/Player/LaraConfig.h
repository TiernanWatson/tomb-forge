#pragma once

#include <unordered_map>
#include <vector>

#include "Engine/Animation/AnimationSet.h"
#include "Engine/Assets/AssetId.h"
#include "Engine/Player/LaraEnums.h"

namespace TombForge
{
    // Used to transition between animation sets
    struct AnimSetEntryKey
    {
        AssetId fromAnimSetId{};
        AssetId toAnimSetId{};
        AnimSetTransition transition{};
    };

    struct LaraConfig
    {
        std::unordered_map<LaraState, AssetId> animSetsForStates{}; // Indexed by LaraState enum
        std::vector<AnimSetEntryKey> animSetEntries{}; // Special case transitions, not always necessary

        std::vector<AssetId> feetSfx{}; // Footstep sounds to randomly choose from
        std::vector<AssetId> jumpSfx{}; // Jump sound effects
        std::vector<AssetId> swooshSfx{}; // Sword swoosh sound effects
        std::vector<AssetId> climbupSfx{}; // Climb up sound effects
        std::vector<AssetId> handSfx{}; // Hand grab sound effects

        glm::vec3 ledgeReachOffset{ 0.0f, -2.2f, -0.65f }; // Offset to apply when reaching for a ledge
        glm::vec3 ledgeGrabOffset{ 0.0f, -1.75f, -0.55f }; // Offset to apply when grabbing a ledge
        glm::vec3 ledgeHangOffset{ 0.0f, -2.25f, 0.01f }; // Offset to apply when hanging from a ledge

        float walkSpeed{ 1.355f }; // Speed Lara moves when walking
        float runSpeed{ 3.5f }; // Speed Lara moves when running
        float turnRate{ 16.0f }; // How quickly the slerping turns Lara to face movement direction
        float walkThreshold{ 0.75f }; // Input magnitude threshold to switch from walk to run
        float deadZone{ 0.1f }; // Input magnitude below which Lara does not move

        float jumpHeight{ 1.0f }; // Maximum height Lara can jump (holding jump button)
        float jumpDistance{ 3.0f }; // Maximum horizontal distance Lara can jump (holding jump button)
        float safeFallDistance{ 3.0f }; // Distance Lara can fall without taking damage
        float deathFallDistance{ 10.0f }; // Distance Lara can fall before dying
        float gravity{ -20.0f }; // Gravity applied to Lara when in air (holding jump and is negative usually)

        AssetId modelId{};
    };
}
