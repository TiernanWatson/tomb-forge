#include "Engine/Player/States/ClimbState.h"

#include <glm/gtx/quaternion.hpp>

#include "Engine/Player/Input.h"
#include "Engine/Player/LaraController.h"

namespace TombForge
{
    static constexpr float CorrectionLerpRate = 15.0f;
    static constexpr float ClimbUpTransitionTime = 5.0f; // TODO: Make configurable per animation

    ClimbState::ClimbState(const glm::vec3& grabOffset, const glm::vec3& hangOffset)
        : m_grabOffset{ grabOffset }, m_hangOffset{ hangOffset }
    {
    }

    void ClimbState::Begin(LaraController& lara)
    {
        lara.SetPosition(lara.GetNearestLedgePoint() + m_grabOffset);
        lara.SetRootMotion(RootMotionMode::On);
        lara.SetCollidesWithWorld(false);
        m_wantsClimbUp = false;
    }

    void ClimbState::PreAnimationUpdate(LaraController& lara, float deltaTime)
    {
        m_velocity = 0.0f;

        if (Input::GetKey(GLFW_KEY_D, GLFW_PRESS))
        {
            m_velocity += 1.0f;
        }

        if (Input::GetKey(GLFW_KEY_A, GLFW_PRESS))
        {
            m_velocity += -1.0f;
        }

        lara.SetTargetVelocity(glm::vec3{ m_velocity, 0.0f, 0.0f });

        if (Input::GetKey(GLFW_KEY_SPACE, GLFW_PRESS))
        {
            m_wantsClimbUp = true;
        }

        if (lara.GetAnimationTag() == "Hang")
        {
            const glm::vec3 targetPosition = lara.GetNearestLedgePoint() + m_hangOffset;
            const glm::quat targetRotation = glm::quatLookAt(-lara.GetLedgeForward(), glm::vec3{0.0f,1.0f,0.0f});
            lara.LerpPosition(targetPosition, deltaTime * CorrectionLerpRate);
        }
    }

    void ClimbState::PostAnimationUpdate(LaraController& lara, float deltaTime)
    {
        const glm::vec3 rootMove = lara.GetAnimPlayer().RootDelta() / deltaTime;
        const glm::vec3 actualVel{ rootMove.x, rootMove.z, -rootMove.y };
        lara.SetVelocity(lara.GetRotation() * actualVel);
    }

    void ClimbState::Exit(LaraController& lara)
    {
        lara.SetCollidesWithWorld(true);
    }

    LaraState ClimbState::ShouldTransition(LaraController& lara)
    {
        if (Input::GetKey(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS))
        {
            return LARA_STATE_AIR;
        }
        else if (lara.GetAnimationTag() == "ClimbUp" && lara.GetAnimPlayer().TimeLeft() < ClimbUpTransitionTime)
        {
            return LARA_STATE_LOCOMOTION;
        }
        return LaraBaseState::ShouldTransition(lara);
    }
}
