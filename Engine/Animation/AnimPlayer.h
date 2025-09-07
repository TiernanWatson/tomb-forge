#pragma once

#include <memory>

#include "Core/Debug.h"
#include "Core/Maths/Transform.h"
#include "Engine/Animation/Animation.h"
#include "Engine/Animation/Skeleton.h"

namespace TombForge
{
    enum class RootMotionMode : uint8_t
    {
        Off,
        PositionOnly,
        On
    };

    struct Pose
    {
        std::vector<Transform> bones{};
    };

    /// <summary>
    /// Computes bone matrices from an animation and a skeleton definition
    /// </summary>
    class AnimPlayer
    {
    public:
        void Play(std::shared_ptr<const Animation> animPlayer, bool loop = false);

        void BlendTo(std::shared_ptr<const Animation> animPlayer, float frames, bool loop = false);

        void Process(float deltaTime);

        // Animation Data

        inline glm::vec3 RootDelta() const
        {
            return m_rootDelta;
        }

        inline glm::quat RootRotDelta() const
        {
            return m_rootRotDelta;
        }

        inline const std::vector<glm::mat4>& FinalBoneMatrices() const
        {
            return m_finalMatrices;
        }

        // Play Data

        void SetSkeleton(std::shared_ptr<const Skeleton> skeleton);

        inline RootMotionMode GetRootMotionMode() const
        {
            return m_rootMotionMode;
        }

        inline void SetRootMotionMode(RootMotionMode mode)
        {
            m_rootMotionMode = mode;
        }

        inline bool IsAnimation(const std::string& name) const
        {
            return m_currentAnim.animation && m_currentAnim.animation->name == name;
        }

        inline float CurrentTime() const
        {
            return m_currentAnim.currentFrame;
        }

        inline float TimeLeft() const
        {
            float currentTime = CurrentTime();
            float totalTime = m_currentAnim.animation->length;
            return totalTime - currentTime;
        }

        inline bool IsLooping() const
        {
            return m_currentAnim.shouldLoop;
        }

        inline void ShouldLoop(bool value)
        {
            m_currentAnim.shouldLoop = value;
        }

        inline bool IsBlending() const
        {
            return m_isBlending;
        }

        // Checkers

        inline bool AnimHasRootMotion() const
        {
            return m_currentAnim.animation->hasRootMotion;
        }

        inline bool IsValid() const
        {
            return m_skeleton != nullptr 
                && (m_currentAnim.animation == nullptr || m_currentAnim.animation->keys.size() == m_skeleton->bones.size());
        }

    private:
        struct AnimPlaybackInfo
        {
            std::shared_ptr<const Animation> animation{};

            glm::vec3 previousRootPosition{};

            glm::quat previousRootRotation{};

            float currentFrame{};

            float previousFrame{};

            int loopCount{};

            bool shouldLoop{};

            glm::vec3 CalculateRootDelta(glm::vec3 newPosition);

            glm::quat CalculateRootRotDelta(const glm::quat& newRot);

            void AdvanceFrame(float deltaTime);

            void Clear();
        };

        glm::vec3 GetPosition(const std::vector<PositionKey>& positions, float frame, glm::vec3 fallback) const;

        glm::vec3 GetScale(const std::vector<ScaleKey>& scales, float frame, glm::vec3 fallback) const;

        glm::quat GetRotation(const std::vector<RotationKey>& rotations, float frame, glm::quat fallback) const;

        void TriggerEvents(const std::vector<EventKey>& events, float frame);

        std::shared_ptr<const Skeleton> m_skeleton{};

        std::vector<glm::mat4> m_finalMatrices{};

        Pose m_finalPose{}; // Used by Process to store the final computed pose
        Pose m_blendPose{}; // Either the last animation or last computed pose if interrupted

        std::vector<glm::vec3> m_defaultPositions{};
        std::vector<glm::quat> m_defaultRotations{};
        std::vector<glm::vec3> m_defaultScales{};

        AnimPlaybackInfo m_currentAnim{};
        AnimPlaybackInfo m_previousAnim{}; // For blending

        glm::vec3 m_rootDelta{}; // Used for storing root motion
        glm::quat m_rootRotDelta{};

        std::function<void(AnimEvent)> m_eventCallback{};

        size_t m_lastEvent{};

        float m_blendTime{};

        RootMotionMode m_rootMotionMode{ RootMotionMode::PositionOnly };

        bool m_isBlending{};
        bool m_wasInterrupted{};
    };
}

