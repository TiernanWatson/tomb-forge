#pragma once

#include "../Animation/AnimPlayer.h"
#include "LaraEnums.h"

namespace TombForge
{
    struct Lara;

    class PhysicsInterface;

    /// <summary>
    /// Hides away the exposed data struct to make gameplay code cleaner and safer
    /// </summary>
    class LaraController
    {
    public:
        LaraController(Lara* lara, PhysicsInterface* physics);
        LaraController(const LaraController&) = delete;
        LaraController(LaraController&&) = delete;

        // Animations

        void SetAnimation(LaraAnim id, float fadeTime = 0.0f, bool loop = true);

        void SetRootMotion(RootMotionMode mode);

        float AnimTimeLeft() const;

        RootMotionMode GetRootMotionMode() const;

        glm::vec3 RootDelta() const;

        glm::quat RootRotDelta() const;

        LaraAnim CurrentAnim() const;

        // Movement

        void SetVelocity(const glm::vec3& velocity);

        void SetRotation(const glm::quat& rotation);

        void SetRotation(const glm::vec3& eulers);

        void SetPosition(const glm::vec3& position);

        void SetColliderOffset(const glm::vec3& offset);

        void SetCollidesWithWorld(bool value);

        glm::vec3 GetVelocity() const;

        glm::quat GetRotation() const;

        glm::vec3 GetPosition() const;

        glm::vec3 GetForward() const;

        bool IsGrounded() const;

        // Camera

        float CameraYaw() const;

        float CameraPitch() const;

    private:
        Lara* m_lara{};
        PhysicsInterface* m_physics{};
    };
}

