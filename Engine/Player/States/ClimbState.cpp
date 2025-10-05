#include "Engine/Player/States/ClimbState.h"

#include "Core/Maths/Maths.h"
#include "Engine/Player/Input.h"

namespace TombForge
{
    void ClimbState::Begin(LaraController& lara)
    {
        lara.SetRootMotion(RootMotionMode::On);
        m_wantsClimbUp = false;
    }

    void ClimbState::PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics)
    {
        
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

        if (Input::GetKey(GLFW_KEY_SPACE, GLFW_PRESS))
        {
            m_wantsClimbUp = true;
        }
    }

    void ClimbState::PostAnimationUpdate(LaraController& lara, float deltaTime)
    {
        if (true || lara.GetRootMotionMode() != RootMotionMode::Off)
        {
            const glm::vec3 rootMove = lara.GetAnimPlayer().RootDelta() / deltaTime;
            const glm::vec3 actualVel{ rootMove.x, rootMove.z, -rootMove.y };
            lara.SetVelocity(lara.GetRotation() * actualVel);
        }
    }

    void ClimbState::Exit(LaraController& lara)
    {
    }

    LaraState ClimbState::ShouldTransition(LaraController& lara)
    {
        return LaraBaseState::ShouldTransition(lara);
    }
}
