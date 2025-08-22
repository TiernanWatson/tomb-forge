#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace TombForge
{
    enum AnimEvent : uint8_t
    {
        ANIM_EVENT_GENERIC,

        ANIM_EVENT_LEFT_FOOT,
        ANIM_EVENT_RIGHT_FOOT,
        ANIM_EVENT_LEFT_HAND,
        ANIM_EVENT_RIGHT_HAND,

        ANIM_EVENT_GROUND_CONTACT,
        ANIM_EVENT_LEFT_GROUND,

        ANIM_EVENT_STATE_TRANSITION,

        ANIM_EVENT_ROOT_MOVE_OFF,
        ANIM_EVENT_ROOT_MOVE_ON,

        ANIM_EVENT_COLLISION_ON,
        ANIM_EVENT_COLLISION_OFF,

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

    struct Animation
    {
        static constexpr float DefaultFrameRate{ 30.0f };

        std::string name{};

        std::vector<BoneKeys> keys{}; // The index corresponds to the bone ID

        std::vector<EventKey> events{};

        float length{}; // Total length of animation in frames

        float framerate{ DefaultFrameRate };

        bool hasRootMotion{}; // Extract root movement and don't apply to skeleton

        bool Load();

        bool SaveBinary() const;
    };

    std::string AnimEventToString(AnimEvent event);
}

