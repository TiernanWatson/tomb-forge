#include "Engine/Player/States/LocomotionState.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/Input.h"

namespace TombForge
{
    void LocomotionState::PreAnimationUpdate(LaraController& lara, float deltaTime)
    {
        const float forwardKey = Input::GetKey(GLFW_KEY_W, GLFW_PRESS) ? -1.0f : 0.0f;
        const float backKey = Input::GetKey(GLFW_KEY_S, GLFW_PRESS) ? 1.0f : 0.0f;
        const float leftKey = Input::GetKey(GLFW_KEY_A, GLFW_PRESS) ? -1.0f : 0.0f;
        const float rightKey = Input::GetKey(GLFW_KEY_D, GLFW_PRESS) ? 1.0f : 0.0f;

        const bool isWalk = Input::GetKey(GLFW_KEY_LEFT_ALT, GLFW_PRESS);

        m_wantsToJump = Input::GetKey(GLFW_KEY_SPACE, GLFW_PRESS);
        if (m_wantsToJump)
        {
            lara.JumpRequested();
        }

        m_moveInput = { leftKey + rightKey, 0.0f, forwardKey + backKey };
        if (isWalk)
        {
            m_moveInput *= 0.5f * (m_walkThreshold + m_runThreshold);
        }

        glm::vec3 targetMove = m_moveInput;
        if (glm::length(targetMove) > 1.0f)
        {
            targetMove = glm::normalize(targetMove);
        }
        targetMove *= isWalk ? m_walkSpeed : m_runSpeed;

        glm::quat cameraYRotation{ { 0.0f, lara.CameraYaw(), 0.0f } };
        targetMove = cameraYRotation * targetMove;

        m_desiredVelocity = targetMove;
        lara.SetTargetVelocity(m_desiredVelocity);

        m_targetSpeed = glm::length(targetMove);
    }

    void LocomotionState::PostAnimationUpdate(LaraController& lara, float deltaTime)
    {
        // Rotation
        if (glm::length(m_desiredVelocity) > 0.01f)
        {
            const float angle = glm::atan(m_desiredVelocity.x, m_desiredVelocity.z);
            const glm::quat targetRotation({ 0.0f, angle, 0.0f });
            lara.SetRotation(glm::slerp(lara.GetRotation(), targetRotation, deltaTime * 30.0f));
        }

        // Movement
        glm::vec3 actualVelocity = m_desiredVelocity;

        if (lara.GetRootMotionMode() != RootMotionMode::Off)
        {
            // todo: correct orientations so that root delta can be used as-is
            glm::vec3 rootMove = lara.GetAnimPlayer().RootDelta() / deltaTime;
            actualVelocity.x = rootMove.x;
            actualVelocity.z = -rootMove.y;

            // Root motion doesn't rotate automatically to transform
            actualVelocity = lara.GetRotation() * actualVelocity;
        }

        actualVelocity.y = -9.8f * deltaTime;
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

    bool LocomotionState::FindLedge(glm::vec3 position, glm::vec3 direction, PhysicsInterface& physics)
    {
        for (float height = 1.0f; height <= 4.0f; height += 0.25f)
        {
            glm::vec3 rayOrigin = position + glm::vec3{ 0.0f, height, 0.0f };
            glm::vec3 rayDirection = direction * 6.0f;

            HitResult result{};
            if (physics.Raycast({ rayOrigin, rayDirection }, result))
            {
                rayOrigin.x = result.point.x;
                rayOrigin.z = result.point.z;
                rayOrigin.y = result.point.y + 0.25f;

                rayDirection = glm::vec3(0.0f, 0.25f, 0.0f);

                HitResult result2{};
                if (physics.Raycast({ rayOrigin, rayDirection }, result))
                {
                    m_foundLedgeForJump = true;

                    m_ledgePosition = { result.point.x, result2.point.y, result.point.z };
                    m_ledgeForward = -result.normal;

                    return true;
                }
            }
        }

        return false;
    }
}
