#include "Engine/Player/States/AirState.h"

#include <glm/gtx/vector_angle.hpp>

#include "Core/Debug.h"
#include "Engine/Assets/AssetRegistry.h"
#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/Input.h"
#include "Engine/Rendering/JoltDebugRenderer.h"

namespace TombForge
{
    namespace
    {
        const glm::vec4 s_debugRayColor{ 1.0f, 0.0f, 0.0f, 1.0f };
        const glm::vec4 s_debugFoundColor{ 0.0f, 1.0f, 0.0f, 1.0f };

        constexpr float JumpVelocityInterpTime{ 0.1f }; // Time to reach target jump velocity
    }

    void AirState::Begin(LaraController& lara)
    {
        m_isReaching = false;
        m_foundLedge = false;

        if (lara.GetAnimPlayer().IsAnimation("JumpForwardL"))
        {
            float t = sqrtf(2.0f * m_jumpHeight / -m_gravity);
            float speedXZ = m_jumpDistance / (2.0f * t);
            float speedY = (m_jumpHeight / t) - (0.5f * m_gravity * t);

            m_targetVelocity = -lara.GetForward() * speedXZ;
            m_targetVelocity.y = speedY;
            m_initialVelocity = lara.GetVelocity();
            m_isInterping = true;
            m_jumpInterp = 0.0f;
        }
        else
        {
            m_isInterping = false;
            m_jumpInterp = 1.0f;
        }
    }

    void AirState::PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics)
    {
        if (m_isReaching)
        {
            const glm::vec3 forwardOrigin = lara.GetPosition() + glm::vec3{0.0f, 1.9f, 0.0f};
            const glm::vec3 forwardDir = -lara.GetForward();
            const Ray forwardRay{ forwardOrigin, forwardDir };
            HitResult resultForward{};

            DEBUG_RAY(physics, forwardRay, s_debugRayColor);

            if (physics.Raycast(forwardRay, resultForward))
            {
                const glm::vec3 downOrigin = resultForward.point + forwardDir * 0.1f + glm::vec3{ 0.0f, 0.2f, 0.0f };
                const glm::vec3 downDir = glm::vec3{ 0.0f, -0.2f, 0.0f };
                const Ray downRay{ downOrigin, downDir };
                HitResult resultDown{};

                DEBUG_RAY(physics, downRay, s_debugRayColor);

                if (physics.Raycast(downRay, resultDown))
                {
                    const float surfaceAngle = angle(glm::vec3{ 0.0f, 1.0f, 0.0f }, resultDown.normal);

                    if (surfaceAngle < glm::radians(10.0f))
                    {
                        const glm::vec3 ledgePoint{ resultForward.point.x, resultDown.point.y, resultForward.point.z };

                        m_foundLedge = true;
                        m_ledgePosition = ledgePoint + glm::vec3{ 0.0f, -2.1f, 0.0 } + resultForward.normal * 0.125f;
                        m_ledgeForward = -resultForward.normal;

                        physics.SetPlayerCollidesWorld(false);
                    }
                }
            }
        }
    }

    void AirState::PreAnimationUpdate(LaraController& lara, float deltaTime)
    {
        // todo: create movement component to handle this
        glm::vec3 input{};
        if (Input::GetKey(GLFW_KEY_W, GLFW_PRESS))
        {
            input.z -= 1.0f;
        }
        if (Input::GetKey(GLFW_KEY_S, GLFW_PRESS))
        {
            input.z += 1.0f;
        }
        if (Input::GetKey(GLFW_KEY_A, GLFW_PRESS))
        {
            input.x -= 1.0f;
        }
        if (Input::GetKey(GLFW_KEY_D, GLFW_PRESS))
        {
            input.x += 1.0f;
        }
        if (glm::length(input) > 1.0f)
        {
            input = glm::normalize(input);
        }
        input *= 3.5f;
        lara.SetTargetVelocity(input);
        if (m_isInterping)
        {
            m_jumpInterp += deltaTime / JumpVelocityInterpTime;
            if (m_jumpInterp >= 1.0f)
            {
                m_jumpInterp = 1.0f;
                m_isInterping = false;
            }
            lara.SetVelocity(glm::mix(m_initialVelocity, m_targetVelocity, m_jumpInterp));
        }
        else
        {
            glm::vec3 velocity = lara.GetVelocity();
            velocity.y += m_gravity * deltaTime;

            lara.SetVelocity(velocity);

            if (Input::GetKey(GLFW_KEY_E, GLFW_PRESS))
            {
                m_isReaching = true;
                lara.SetColliderOffset(lara.GetForward() * 0.25f);
            }
        }
    }

    void AirState::Exit(LaraController& lara)
    {
        lara.SetColliderOffset({ 0.0f, 0.0f, 0.0f });
    }

    LaraState AirState::ShouldTransition(LaraController& lara)
    {
        if (m_foundLedge)
        {
            lara.SetVelocity({});
            lara.SetColliderOffset({});
            lara.SetPosition(m_ledgePosition);
            return LARA_STATE_CLIMB;
        }

        if (lara.GetVelocity().y < 0.0f && lara.IsGrounded())
        {
            return LARA_STATE_LOCOMOTION;
        }

        return LaraBaseState::ShouldTransition(lara);
    }
}
