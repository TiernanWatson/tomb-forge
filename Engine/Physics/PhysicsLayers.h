#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace TombForge
{
    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NonMoving{ 0 };
        static constexpr JPH::BroadPhaseLayer Moving{ 1 };
        static constexpr JPH::BroadPhaseLayer Character{ 2 };
        static constexpr JPH::BroadPhaseLayer Trigger{ 3 };
        static constexpr JPH::uint NumLayers{ 4 };
    };

    namespace ObjectLayers
    {
        static constexpr JPH::ObjectLayer NonMoving{ 0 };
        static constexpr JPH::ObjectLayer Moving{ 1 };
        static constexpr JPH::ObjectLayer Character{ 2 };
        static constexpr JPH::ObjectLayer Trigger{ 3 };
        static constexpr JPH::ObjectLayer NumLayers{ 4 };
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            m_objectToBroadPhase[ObjectLayers::NonMoving] = BroadPhaseLayers::NonMoving;
            m_objectToBroadPhase[ObjectLayers::Moving] = BroadPhaseLayers::Moving;
            m_objectToBroadPhase[ObjectLayers::Character] = BroadPhaseLayers::Character;
            m_objectToBroadPhase[ObjectLayers::Trigger] = BroadPhaseLayers::Trigger;
        }

        inline virtual JPH::uint GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NumLayers;
        }

        inline virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < ObjectLayers::NumLayers);
            return m_objectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        inline virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
        {
            switch ((JPH::BroadPhaseLayer::Type)inLayer)
            {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NonMoving:
                return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::Moving:
                return "MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::Character:
                return "CHARACTER";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::Trigger:
                return "TRIGGER";
            default:
            {
                JPH_ASSERT(false);
                return "INVALID";
            }
            }
        }
#endif

    private:
        JPH::BroadPhaseLayer m_objectToBroadPhase[ObjectLayers::NumLayers]{};
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        inline virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case ObjectLayers::NonMoving:
                return inLayer2 == BroadPhaseLayers::Moving;
            case ObjectLayers::Character:
                return inLayer2 == BroadPhaseLayers::Character;
            case ObjectLayers::Moving:
                return m_playerCollidesWithWorld && inLayer2 != BroadPhaseLayers::Character;
            case ObjectLayers::Trigger:
                return inLayer2 != BroadPhaseLayers::Character; // Triggers don't collide with the player
            default:
            {
                JPH_ASSERT(false);
                return false;
            }
            }
        }

        inline void SetPlayerCollides(bool value)
        {
            m_playerCollidesWithWorld = value;
        }

    private:
        bool m_playerCollidesWithWorld{ true };
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        inline virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case ObjectLayers::NonMoving:
                return inObject2 == ObjectLayers::Moving;
            case ObjectLayers::Character:
                return inObject2 == ObjectLayers::Character;
            case ObjectLayers::Moving:
                return m_playerCollidesWithWorld && inObject2 != ObjectLayers::Character;
            case ObjectLayers::Trigger:
                return inObject2 != ObjectLayers::Character; // Triggers don't collide with the player
            default:
            {
                JPH_ASSERT(false);
                return false;
            }
            }
        }

        inline void SetPlayerCollides(bool value)
        {
            m_playerCollidesWithWorld = value;
        }

    private:
        bool m_playerCollidesWithWorld{ true };
    };
}
