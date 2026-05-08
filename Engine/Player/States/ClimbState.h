#pragma once

#include <glm/vec3.hpp>

#include "Engine/Player/States/LaraState.h"

namespace TombForge
{
    class ClimbState : public LaraBaseState
    {
    public:
        ClimbState(const glm::vec3& grabOffset, const glm::vec3& hangOffset);

        virtual void Begin(LaraController& lara) override;
        virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) override;
        virtual void PostAnimationUpdate(LaraController& lara, float deltaTime) override;
        virtual void Exit(LaraController& lara) override;

        virtual LaraState ShouldTransition(LaraController& lara) override;
        
        bool IsClimbingUp() const { return m_wantsClimbUp; }

        void SetGrabOffset(const glm::vec3& offset) { m_grabOffset = offset; }
        void SetHangOffset(const glm::vec3& offset) { m_hangOffset = offset; }

    private:
        glm::vec3 m_warpOffsetGrab{};
        glm::vec3 m_warpOffsetClimb{};
        glm::vec3 m_grabOffset{};
        glm::vec3 m_hangOffset{};

        float m_velocity{};
        bool m_wantsClimbUp{};
    };
}

