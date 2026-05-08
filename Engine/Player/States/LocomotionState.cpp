#include "Engine/Player/States/LocomotionState.h"

#include <glm/vec3.hpp>

#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/Input.h"
#include "Engine/Player/LaraController.h"

namespace TombForge
{
    LocomotionState::LocomotionState(float runSpeed, float walkSpeed, float turnRate, float walkThreshold, float runThreshold)
        : m_runSpeed{ runSpeed }
        , m_walkSpeed{ walkSpeed }
        , m_slerpRate{ turnRate }
        , m_walkThreshold{ walkThreshold }
        , m_runThreshold{ runThreshold }
    {
    }

    void LocomotionState::PreAnimationUpdate(LaraController& lara, float deltaTime)
    {
        // Right-handed coordinate system, so invert forward
        const float forwardKey = Input::GetKey(GLFW_KEY_W, GLFW_PRESS) ? -1.0f : 0.0f;
        const float backKey = Input::GetKey(GLFW_KEY_S, GLFW_PRESS) ? 1.0f : 0.0f;
        const float leftKey = Input::GetKey(GLFW_KEY_A, GLFW_PRESS) ? -1.0f : 0.0f;
        const float rightKey = Input::GetKey(GLFW_KEY_D, GLFW_PRESS) ? 1.0f : 0.0f;
        const bool isWalk = Input::GetKey(GLFW_KEY_LEFT_ALT, GLFW_PRESS);

        if (Input::GetKey(GLFW_KEY_SPACE, GLFW_PRESS))
        {
            lara.JumpRequested();
        }

        glm::vec3 targetMove{ leftKey + rightKey, 0.0f, forwardKey + backKey };
        if (glm::length(targetMove) > 1.0f)
        {
            targetMove = glm::normalize(targetMove);
        }
        targetMove *= isWalk ? m_walkSpeed : m_runSpeed;

        glm::quat cameraYRotation{ { 0.0f, lara.CameraYaw(), 0.0f } };
        targetMove = cameraYRotation * targetMove;

        lara.SetTargetVelocity(targetMove);
    }

    void LocomotionState::PostAnimationUpdate(LaraController& lara, float deltaTime)
    {
        // Rotation
        const glm::vec3 targetVelocity = lara.GetTargetVelocity();
        if (glm::length(targetVelocity) > 0.01f)
        {
            const float angle = glm::atan(targetVelocity.x, targetVelocity.z);
            const glm::quat targetRotation(glm::vec3{ 0.0f, angle, 0.0f });
            lara.SlerpRotation(targetRotation, deltaTime * m_slerpRate);
        }

        // Movement
        glm::vec3 actualVelocity{};
        if (lara.GetRootMotionMode() != RootMotionMode::Off)
        {
            // todo: correct orientations so that root delta can be used as-is
            glm::vec3 rootMove = lara.GetAnimPlayer().RootDelta() / deltaTime;
            actualVelocity.x = rootMove.x;
            actualVelocity.z = -rootMove.y;

            // Root motion doesn't rotate automatically to transform
            actualVelocity = lara.GetRotation() * actualVelocity;
        }
        else
        {
            actualVelocity = targetVelocity;
        }
        lara.SetVelocity(actualVelocity);
    }

    LaraState LocomotionState::ShouldTransition(LaraController& lara)
    {
        // todo: remove the time left check here and so it is handled in a more robust way
        if (!lara.IsGrounded() || (lara.GetAnimPlayer().IsAnimation("RunCompressToJumpL") && lara.GetAnimPlayer().TimeLeft() < 1.0f))
        {
            return LARA_STATE_AIR;
        }
        return LaraBaseState::ShouldTransition(lara);
    }
}
