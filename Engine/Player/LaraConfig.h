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
        AssetId modelId{};
    };
}
