#pragma once

#include "Engine/Player/States/LaraState.h"

namespace TombForge
{
    class AirState : public LaraBaseState
    {
    public:
        virtual void Begin(LaraController& lara) override;

        virtual void PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics) override;

        virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) override;

        virtual void Exit(LaraController& lara) override;

        virtual LaraState ShouldTransition(LaraController& lara) override;

    private:
        glm::vec3 m_targetPosition{};
        glm::vec3 m_targetForward{};
        glm::vec3 m_ledgePosition{};
        glm::vec3 m_ledgeForward{};
        glm::vec3 m_targetVelocity{};
        glm::vec3 m_initialVelocity{};

        float m_jumpInterp{};
        float m_jumpDistance{ 4.5f };
        float m_jumpHeight{ 1.6f };
        float m_speedBoost{ 1.0f };
        float m_gravity{ -20.0f };
        bool m_isReaching{};
        bool m_foundLedge{};
        bool m_isInterping{};
    };
}

