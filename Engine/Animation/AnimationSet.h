#pragma once

#include <memory>
#include <vector>

#include "Engine/Animation/Animation.h"
#include "Engine/Assets/AssetId.h"

namespace TombForge
{
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
        uint32_t defaultAnimation{};
        float defaultBlendTime{};
        float defaultTargetFrame{};
        bool defaultShouldBlend{};
        bool defaultShouldLoop{};
    };
}
