#pragma once

#include <memory>
#include <vector>

#include "Core/Debug.h"
#include "Engine/Animation/Animation.h"
#include "Engine/Assets/AssetId.h"

namespace TombForge
{
    // Used to correct offset errors in animations
    struct BoneWarp
    {
        uint32_t animationIndex{}; // Which animation to warp, corresponds to animation array index
        glm::vec3 offset{}; // How much to move the bone by
        uint8_t boneId{}; // Bone to warp, often the root or hips
        float startFrame{}; // When to start the warp, in frames
        float endFrame{}; // When to end the warp, in frames (== start frame means instant)
        bool reverse{}; // If true, start at full offset and end at no offset
    };

    struct AnimSetTransition
    {
        struct Condition
        {
            enum class Type : uint8_t
            {
                OnFinish,
                SpeedGreater,
                SpeedLess,
                TargetSpeedGreater,
                TargetSpeedLess,
                OnGround,
                OffGround,
                TargetDirectionGreater,
                TargetDirectionLess,
                WantsJump,
                TimeLeft,
                IsReaching,
                ClimbUp,
                Count
            };

            Type condition{};
            float threshold{}; // Used only for relevant conditions
        };

        std::vector<Condition> conditions{}; // All conditions must be met
        std::vector<uint32_t> fromAnimations{}; // Index into animation set

        uint32_t toAnimation{}; // Index into animation set

        float blendDuration{}; // Duration of blend to target animation in frames
        float targetFrame{}; // Frame to play from in target animation
        float minFramesElapsed{}; // Minimum time before transition can occur

        bool shouldBlend{}; // Whether or not to blend to target animation
        bool loop{}; // Should target animation loop on play
        bool snapRoot{}; // Only relevant if blending is on, if true, don't interpolate root

        BlendCurve blendCurve{ BlendCurve::Linear };

        inline bool ContainsFromAnimation(uint32_t animIndex) const
        {
            for (const auto& fromAnim : fromAnimations)
            {
                if (fromAnim == animIndex)
                {
                    return true;
                }
            }
            return false;
        }
    };

    // Contains a collection of animations and the conditions for transitioning between them
    struct AnimationSet : public AssetBase
    {
        using Transition = AnimSetTransition;

        std::vector<std::shared_ptr<Animation>> animations{};
        std::vector<Transition> transitions{};
        std::vector<BoneWarp> boneWarps{}; // Bone offsets that should be applied for animations
        std::vector<std::string> animTags{}; // One tag per animation, e.g. "ClimbUp"

        uint32_t defaultAnimation{};
        float defaultBlendTime{};
        float defaultTargetFrame{};
        bool defaultShouldBlend{};
        bool defaultShouldLoop{};
        bool defaultShouldSnapRoot{};

        const std::string& GetTag(uint32_t animIndex) const
        {
            static const std::string emptyTag{};
            if (animIndex < animTags.size())
            {
                return animTags[animIndex];
            }
            return emptyTag;
        }
    };
}
