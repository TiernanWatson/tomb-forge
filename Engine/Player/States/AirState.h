#pragma once

#include <glm/vec3.hpp>

#include "Engine/Player/States/LaraState.h"

namespace TombForge
{
    class AirState : public LaraBaseState
    {
    public:
        AirState(const glm::vec3& reachOffset,
            float jumpDistance,
            float jumpHeight,
            float gravity,
            float safeFallDistance,
            float deathFallDistance);

        virtual void Begin(LaraController& lara) override;
        virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) override;
        virtual void PostPhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics) override;
        virtual void Exit(LaraController& lara) override;

        virtual LaraState ShouldTransition(LaraController& lara) override;

        bool IsReaching() const { return m_isReaching; }

        void SetReachOffset(const glm::vec3& offset) { m_reachOffset = offset; }
        void SetJumpDistance(float distance) { m_jumpDistance = distance; }
        void SetJumpHeight(float height) { m_jumpHeight = height; }
        void SetGravity(float gravity) { m_gravity = gravity; }
        void SetSafeFallDistance(float distance) { m_safeFallDistance = distance; }
        void SetDeathFallDistance(float distance) { m_deathFallDistance = distance; }

    private:
        glm::vec3 m_targetVelocity{};
        glm::vec3 m_initialVelocity{};
        glm::vec3 m_reachOffset{}; // Used in calculation of ledge grab target

        float m_jumpInterp{};
        float m_jumpDistance{};
        float m_jumpHeight{};
        float m_gravity{};
        float m_grabTime{}; // Time in air before switching to climb
        float m_timeInAir{};
        float m_fallStartY{};
        float m_safeFallDistance{};
        float m_deathFallDistance{};

        bool m_isReaching{};
        bool m_isInterping{};
    };
}

