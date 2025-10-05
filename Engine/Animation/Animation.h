#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

#include "Engine/Assets/AssetId.h"

namespace TombForge
{
    enum AnimEvent : uint8_t
    {
        ANIM_EVENT_GENERIC,
        ANIM_EVENT_FOOT_SFX,
        ANIM_EVENT_COUNT
    };

    template<typename KeyType>
    struct BoneKey
    {
        KeyType value{};
        float time{};
    };

    using PositionKey = BoneKey<glm::vec3>;
    using RotationKey = BoneKey<glm::quat>;
    using ScaleKey = BoneKey<glm::vec3>;
    using EventKey = BoneKey<uint8_t>;

    struct BoneKeys
    {
        std::vector<PositionKey> positions{};
        std::vector<RotationKey> rotations{};
        std::vector<ScaleKey> scales{};
    };

    struct Animation : public AssetBase
    {
        static constexpr float DefaultFrameRate{ 30.0f };

        std::vector<BoneKeys> keys{}; // The index corresponds to the bone ID
        std::vector<EventKey> events{};
        float length{}; // Total length of animation in frames
        float framerate{ DefaultFrameRate };
        bool hasRootMotion{}; // Extract root movement and don't apply to skeleton
    };

    std::string AnimEventToString(AnimEvent event);
}

