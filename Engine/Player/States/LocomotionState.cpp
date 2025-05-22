#include "LocomotionState.h"

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Input.h"
#include "../Lara.h"
#include "../../Physics/PhysicsInterface.h"

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

        m_targetSpeed = glm::length(targetMove);
	}

    void LocomotionState::UpdateAnimation(LaraController& lara, float deltaTime)
    {
        const float joystickSpeed = glm::length(m_moveInput);
        const bool wantsToRun = joystickSpeed > m_runThreshold;
        const bool wantsToWalk = !wantsToRun && joystickSpeed > m_walkThreshold;
        const bool wantsToIdle = !wantsToRun && !wantsToWalk;

        switch (lara.CurrentAnim())
        {
        case LARA_ANIM_IDLE:
        {
            if (wantsToRun)
            {
                lara.SetAnimation(LARA_ANIM_RUN_START);
            }
            else if (wantsToWalk)
            {
                lara.SetAnimation(LARA_ANIM_WALK_START);
            }
            break;
        }
        case LARA_ANIM_WALK_START:
        {
            if (lara.AnimTimeLeft() < 3.0f)
            {
                lara.SetAnimation(LARA_ANIM_WALK, 3.0f, true);
            }
            else if (wantsToIdle)
            {
                lara.SetAnimation(LARA_ANIM_IDLE, 3.0f, true);
            }
            else if (wantsToRun)
            {
                lara.SetAnimation(LARA_ANIM_RUN_START, 3.0f);
            }
            break;
        }
        case LARA_ANIM_WALK:
        {
            if (wantsToIdle)
            {
                lara.SetAnimation(LARA_ANIM_IDLE, 3.0f, true);
            }
            else if (m_wantsToJump)
            {
                lara.SetAnimation(LARA_ANIM_RUN_TO_JUMP_L, 3.0f);
            }
            break;
        }
        case LARA_ANIM_RUN_START:
        {
            if (lara.AnimTimeLeft() < 3.0f)
            {
                lara.SetAnimation(LARA_ANIM_RUN, 3.0f, true);
            }
            else if (!wantsToRun && !wantsToWalk)
            {
                lara.SetAnimation(LARA_ANIM_IDLE, 3.0f, true);
            }
            break;
        }
        case LARA_ANIM_RUN:
        {
            if (wantsToIdle)
            {
                lara.SetAnimation(LARA_ANIM_IDLE, 3.0f, true);
            }
            else if (m_wantsToJump)
            {
                lara.SetAnimation(LARA_ANIM_RUN_TO_JUMP_L, 3.0f);
            }
            break;
        }
        case LARA_ANIM_RUN_JUMP_L:
        case LARA_ANIM_RUN_JUMP_R:
        case LARA_ANIM_JUMP_TO_FALL:
        case LARA_ANIM_FALL:
        {
            glm::vec3 groundInput = m_desiredVelocity;
            if (wantsToRun)
            {
                lara.SetAnimation(LARA_ANIM_FALL_TO_RUN, 1.0f);
            }
            else
            {
                lara.SetAnimation(LARA_ANIM_IDLE);
            }
            break;
        }
        case LARA_ANIM_FALL_TO_RUN:
        {
            if (lara.AnimTimeLeft() < 3.0f)
            {
                lara.SetAnimation(LARA_ANIM_RUN, 3.0f, true);
            }
            break;
        }
        case LARA_ANIM_CLIMB_UP:
        {
            if (lara.AnimTimeLeft() < 1.0f)
            {
                lara.SetAnimation(LARA_ANIM_IDLE, 0.0f, true);
            }
            break;
        }
        case LARA_ANIM_RUN_TO_JUMP_L:
        case LARA_ANIM_RUN_TO_JUMP_R:
            break;
        default:
            lara.SetAnimation(LARA_ANIM_IDLE);
            break;
        }
    }

    void LocomotionState::PostAnimationUpdate(LaraController& lara, float deltaTime)
    {
        // Rotation
        if (lara.CurrentAnim() == LARA_ANIM_RUN_TURN_L
            || lara.CurrentAnim() == LARA_ANIM_RUN_TURN_R)
        {
            
        }
        else if (glm::length(m_desiredVelocity) > 0.01f)
        {
            float angle = glm::atan(m_desiredVelocity.x, m_desiredVelocity.z);

            const glm::quat targetRotation({ 0.0f, angle, 0.0f });

            lara.SetRotation(glm::slerp(lara.GetRotation(), targetRotation, deltaTime * 30.0f));
        }

        // Movement
        glm::vec3 actualVelocity = m_desiredVelocity;

        if (lara.GetRootMotionMode() != RootMotionMode::Off)
        {
            glm::vec3 rootMove = lara.RootDelta() / deltaTime;
            actualVelocity.x = rootMove.x;
            actualVelocity.z = -rootMove.y;

            // Root motion doesn't rotate automatically to transform
            actualVelocity = lara.GetRotation() * actualVelocity;
        }

        actualVelocity.y = -9.8f * deltaTime;
        lara.SetVelocity(actualVelocity);
    }

    void LocomotionState::PostPhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics)
    {
        if (lara.CurrentAnim() == LARA_ANIM_RUN_TO_JUMP_L && lara.AnimTimeLeft() < 3.0f)
        {
            FindLedge(lara.GetPosition(), -lara.GetForward(), physics);
        }
    }

    LaraState LocomotionState::ShouldTransition(LaraController& lara)
    {
        if (lara.CurrentAnim() == LARA_ANIM_RUN_TO_JUMP_L && lara.AnimTimeLeft() < 3.0f)
        {
            glm::vec3 velocity = lara.GetVelocity();
            velocity.y = 6.0f;
            
            lara.SetVelocity(velocity);

            return LARA_STATE_AIR;
        }
        else if (!lara.IsGrounded() && lara.CurrentAnim() != LARA_ANIM_CLIMB_UP)
        {
            lara.SetAnimation(LARA_ANIM_FALL, 3.0f, true);
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
