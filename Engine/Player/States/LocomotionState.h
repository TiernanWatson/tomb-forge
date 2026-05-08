#pragma once

#include <glm/glm.hpp>

#include "Engine/Player/States/LaraState.h"

namespace TombForge
{
    class LocomotionState : public LaraBaseState
    {
    public:
        LocomotionState(float runSpeed, float walkSpeed, float turnRate, float walkThreshold, float runThreshold);

        virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) override;
        virtual void PostAnimationUpdate(LaraController& lara, float deltaTime) override;

        virtual LaraState ShouldTransition(LaraController& lara) override;

        void SetRunSpeed(float speed) { m_runSpeed = speed; }
        void SetWalkSpeed(float speed) { m_walkSpeed = speed; }
        void SetTurnRate(float rate) { m_slerpRate = rate; }
        void SetWalkThreshold(float threshold) { m_walkThreshold = threshold; }
        void SetRunThreshold(float threshold) { m_runThreshold = threshold; }

    private:
        float m_runSpeed{};
        float m_walkSpeed{};
        float m_slerpRate{};
        float m_walkThreshold{};
        float m_runThreshold{};
    };
}

