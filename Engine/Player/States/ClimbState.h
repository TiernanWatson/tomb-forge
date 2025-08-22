#pragma once

#include "LaraState.h"

namespace TombForge
{
    class ClimbState : public LaraBaseState
    {
    public:
        virtual void Begin(LaraController& lara) override;

        virtual void PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics) override;

        virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) override;

        virtual void UpdateAnimation(LaraController& lara, float deltaTime) override;

        virtual void PostAnimationUpdate(LaraController& lara, float deltaTime) override;

        virtual void Exit(LaraController& lara) override;

        virtual LaraState ShouldTransition(LaraController& lara) override;

    private:
        float m_velocity{};
        bool m_wantsClimbUp{};
    };
}

