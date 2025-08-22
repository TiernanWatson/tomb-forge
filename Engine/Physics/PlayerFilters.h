#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace TombForge
{
    class PlayerBpFilter : public JPH::BroadPhaseLayerFilter
    {
    public:
        inline virtual bool ShouldCollide(JPH::BroadPhaseLayer layer) const override
        {
            return m_canCollide;
        }

        inline void SetCanCollide(bool value)
        {
            m_canCollide = value;
        }

    private:
        bool m_canCollide{ true };
    };

    class PlayerLayerFilter : public JPH::ObjectLayerFilter
    {
    public:
        inline virtual bool ShouldCollide(JPH::ObjectLayer layer) const override
        {
            return m_canCollide;
        }

        inline void SetCanCollide(bool value)
        {
            m_canCollide = value;
        }

    private:
        bool m_canCollide{ true };
    };
}

