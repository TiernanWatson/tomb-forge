#pragma once

#include <cstdint>
#include <vector>

#include <glm/gtc/quaternion.hpp>

namespace TombForge
{
    struct Animation;
    struct Skeleton;

    struct BoneRetargetSettings
    {
        bool usePosition{};
        bool useRotation{ true };
        bool useScale{};
    };

    struct SkeletonRetargetMap
    {
        std::vector<uint8_t> map{}; // Indexed by source bone, value is the target bone
        std::vector<glm::quat> rotations{}; // Indexed by source bone, matching the map
        std::vector<BoneRetargetSettings> settings{}; // Indexed by source bone, matching the map

        SkeletonRetargetMap() = default;
        SkeletonRetargetMap(const Skeleton& source);

        void FillRotations(const Skeleton& source, const Skeleton& target); // To be called after the map is filled
        void RetargetAnimation(const Skeleton& source, const Skeleton& target, const Animation& sourceAnim, Animation& animation) const;
    };
}
