#include "Engine/Player/States/AirState.h"

#include <cmath>

#include <glm/gtx/vector_angle.hpp>

#include "Core/Debug.h"
#include "Engine/Levels/Level.h"
#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/Input.h"
#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/LaraController.h"
#include "Engine/Rendering/JoltDebugRenderer.h"

namespace TombForge
{
    namespace
    {
        constexpr float JumpVelocityInterpTime{ 0.1f }; // Time to reach target jump velocity
        constexpr float ShortJumpGravityMultiplier{ 1.5f };
        constexpr float TerminalVelocity{ -50.0f };
        constexpr float LedgeGrabRayStep{ 0.25f };
        constexpr float LedgeGrabRayDistance{ 8.0f };

        // Returns time to reach target, and sets velocityXZ and velocityY
        bool TryCalculateLedgeJump(
            const glm::vec3& start,
            const glm::vec3& target,
            const glm::vec3& direction,
            float gravity,
            float& velocityXZ,
            float& velocityY,
            float& time)
        {
            glm::vec3 target2D = target;
            target2D.y = 0.0f;

            glm::vec3 start2D = start;
            start2D.y = 0.0f;

            const float distanceXZ = glm::length(target2D - start2D);
            const float displacementY = target.y - start.y;

            const float jumpHeight = displacementY > 1.5f ? displacementY : 1.5f;
            const float timeToApex = sqrtf(2.0f * jumpHeight / -gravity);
            velocityY = -gravity * timeToApex;

            const float discriminant = (velocityY * velocityY) + (2.0f * gravity * displacementY);
            if (discriminant < 0.0f)
            {
                // Ledge is out of reach
                return false;
            }
            time = (-velocityY - sqrtf(discriminant)) / gravity; // Currently assuming we want to grab after apex
            ASSERT(time >= (-velocityY + sqrtf(discriminant)) / gravity, "Expected -ve root to be the larger value");

            velocityXZ = distanceXZ / time;
            return true;
        }
    }

    AirState::AirState(const glm::vec3& reachOffset,
        float jumpDistance,
        float jumpHeight,
        float gravity,
        float safeFallDistance,
        float deathFallDistance)
        : m_reachOffset(reachOffset)
        , m_jumpDistance(jumpDistance)
        , m_jumpHeight(jumpHeight)
        , m_gravity(gravity)
        , m_safeFallDistance(safeFallDistance)
        , m_deathFallDistance(deathFallDistance)
    {
    }

    void AirState::Begin(LaraController& lara)
    {
        m_isReaching = false;
        m_timeInAir = 0.0f;
        m_fallStartY = lara.GetPosition().y;

        const glm::vec3 laraForward = -lara.GetForward();
        const glm::vec3 laraPosition = lara.GetPosition();

        float time = sqrtf(2.0f * m_jumpHeight / -m_gravity);
        float speedXZ = m_jumpDistance / (2.0f * time);
        float speedY = (m_jumpHeight / time) - (0.5f * m_gravity * time);

        if (lara.GetAnimPlayer().IsAnimation("JumpForwardL"))
        {
            auto& physics = lara.GetPhysics();
            for (float height = 0.0f; height <= m_jumpHeight; height += LedgeGrabRayStep)
            {
                const glm::vec3 rayOrigin = laraPosition + glm::vec3{ 0.0f, height, 0.0f };
                const glm::vec3 rayDirection = laraForward * LedgeGrabRayDistance;

                Ray ray{ rayOrigin, rayDirection };
                HitResult result{};
                if (physics.Raycast(ray, result))
                {
                    ColliderHandle colliderData{ result.userData };
                    if (colliderData.bodyType != COLLIDER_LEDGE)
                    {
                        continue;
                    }

                    auto& ledges = lara.GetLevel()->ledges;
                    const glm::vec3 reachOffset = glm::vec3{ 0.0f, -2.2f, 0.0f } + result.normal * (0.65f);
                    const glm::vec3 ledgeStart = ledges[colliderData.index].point;
                    const glm::vec3 ledgeEnd = ledges[ledges[colliderData.index].nextLedge].point;
                    const glm::vec3 ledgeDir = glm::normalize(ledgeEnd - ledgeStart);
                    
                    // Calculate intersection of ledge line and ray line, derived from 2D line equations
                    float numerator = ledgeDir.x * (laraPosition.z - ledgeStart.z) - ledgeDir.z * (laraPosition.x + ledgeStart.x);
                    float denominator = laraForward.x * ledgeDir.z - laraForward.z * ledgeDir.x;
                    float intersectionT = numerator / denominator;
                    if (intersectionT > 0.0f) // Only consider points in front of Lara
                    {
                        glm::vec3 ledgePosition = laraPosition + laraForward * intersectionT;
                        ledgePosition.y = ledgeStart.y;
                        ledgePosition += reachOffset;
                        if (TryCalculateLedgeJump(laraPosition, ledgePosition, laraForward, m_gravity, speedXZ, speedY, time))
                        {
                            m_isReaching = true;
                            m_grabTime = time;
                            break;
                        }
                    }
                }
            }

            m_targetVelocity = laraForward * speedXZ;
            m_targetVelocity.y = speedY;
            m_initialVelocity = lara.GetVelocity();
            m_isInterping = true;
            m_jumpInterp = 0.0f;
        }
        else
        {
            // This just means we've run off a ledge without jumping
            m_isInterping = false;
            m_jumpInterp = 1.0f;
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
            const bool useHighGravity = Input::GetKey(GLFW_KEY_SPACE, GLFW_PRESS) || m_isReaching;
            const float effectiveGravity = useHighGravity ? m_gravity : m_gravity * ShortJumpGravityMultiplier;

            glm::vec3 velocity = lara.GetVelocity();
            velocity.y += effectiveGravity * deltaTime;
            if (velocity.y < TerminalVelocity)
            {
                velocity.y = TerminalVelocity;
            }
            lara.SetVelocity(velocity);
            m_timeInAir += deltaTime;
        }
    }

    void AirState::PostPhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics)
    {
        if (lara.GetVelocity().y < 0.0f && lara.IsGrounded())
        {
            const float fallDistance = std::max(m_fallStartY - lara.GetPosition().y, 0.0f);
            if (fallDistance > m_deathFallDistance)
            {
                lara.SetHealth(0.0f);
            }
            else if (fallDistance > m_safeFallDistance)
            {
                const float healthLoss = (fallDistance - m_safeFallDistance) / (m_deathFallDistance - m_safeFallDistance);
                lara.ModifyHealth(-healthLoss);
            }
        }
    }

    void AirState::Exit(LaraController& lara)
    {
        lara.GetPhysics().SetPlayerCollidesWorld(true);
    }

    LaraState AirState::ShouldTransition(LaraController& lara)
    {
        if (m_isReaching && m_timeInAir >= m_grabTime)
        {
            return LARA_STATE_CLIMB;
        }
        else if (lara.GetVelocity().y < 0.0f && lara.IsGrounded())
        {
            return LARA_STATE_LOCOMOTION;
        }

        return LaraBaseState::ShouldTransition(lara);
    }
}
