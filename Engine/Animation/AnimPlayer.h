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
        void Play(std::shared_ptr<const Animation> animPlayer, bool loop = false, float targetFrame = 0.0f);
        void BlendTo(std::shared_ptr<const Animation> animPlayer, float frames, bool loop = false, float targetFrame = 0.0f);
        void Process(float deltaTime);

        void SetSkeleton(std::shared_ptr<const Skeleton> skeleton);

        glm::vec3 RootDelta() const { return m_rootDelta; }
        glm::quat RootRotDelta() const { return m_rootRotDelta; }
        const std::vector<glm::mat4>& FinalBoneMatrices() const { return m_finalMatrices; }

        RootMotionMode GetRootMotionMode() const { return m_rootMotionMode; }
        void SetRootMotionMode(RootMotionMode mode) { m_rootMotionMode = mode; }
        float CurrentTime() const { return m_currentAnim.currentFrame; }
        void ShouldLoop(bool value) { m_currentAnim.isLooping = value; }

        bool IsAnimation(const std::string& name) const
        {
            return m_currentAnim.clip && m_currentAnim.clip->name == name;
        }

        float TimeLeft() const
        {
            if (!m_currentAnim.clip)
            {
                return 0.0f;
            }
            return m_currentAnim.clip->length - m_currentAnim.currentFrame;
        }

        bool IsValid() const
        {
            return m_skeleton && (!m_currentAnim.clip || m_currentAnim.clip->keys.size() == m_skeleton->bones.size());
        }

    private:
        struct AnimationState
        {
            std::shared_ptr<const Animation> clip{};

            glm::vec3 previousRootPosition{};
            glm::quat previousRootRotation{};

            float currentFrame{};
            float previousFrame{};
            int loopCount{}; // How many times we've looped
            bool isLooping{};

            glm::vec3 CalculateRootDelta(glm::vec3 newPosition);
            glm::quat CalculateRootRotDelta(const glm::quat& newRot);
            void AdvanceFrame(float deltaTime);
            void Clear();
        };

        glm::vec3 GetPosition(const std::vector<PositionKey>& positions, float frame, glm::vec3 fallback, bool looping, bool isRoot = false) const;
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

        AnimationState m_currentAnim{};
        AnimationState m_previousAnim{}; // For blending

        glm::vec3 m_rootDelta{}; // Used for storing root motion
        glm::quat m_rootRotDelta{};

        std::function<void(AnimEvent)> m_eventCallback{};
        size_t m_lastEvent{};

        float m_blendStart{}; // Frame the blend starts from on current animation
        float m_blendTime{}; // Total number of frames to blend over

        RootMotionMode m_rootMotionMode{ RootMotionMode::PositionOnly };

        bool m_isBlending{};
        bool m_wasInterrupted{};
    };
}

