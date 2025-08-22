#pragma once

#include "LaraState.h"

namespace TombForge
{
    class AirState : public LaraBaseState
    {
    public:
        virtual void Begin(LaraController& lara) override;

        virtual void PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics) override;

        virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) override;

        virtual void UpdateAnimation(LaraController& lara, float deltaTime) override;

        virtual void Exit(LaraController& lara) override;

        virtual LaraState ShouldTransition(LaraController& lara) override;

    private:
        glm::vec3 m_targetPosition{};
        glm::vec3 m_targetForward{};
        glm::vec3 m_ledgePosition{};
        glm::vec3 m_ledgeForward{};

        bool m_isReaching{};
        bool m_foundLedge{};
    };
}

