#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../../Core/Maths/Transform.h"

namespace TombForge
{
    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NonMoving{ 0 };
        static constexpr JPH::BroadPhaseLayer Moving{ 1 };
        static constexpr JPH::BroadPhaseLayer Character{ 2 };
        static constexpr JPH::uint NumLayers{ 3 };
    };

    namespace ObjectLayers
    {
        static constexpr JPH::ObjectLayer NonMoving{ 0 };
        static constexpr JPH::ObjectLayer Moving{ 1 };
        static constexpr JPH::ObjectLayer Character{ 2 };
        static constexpr JPH::ObjectLayer NumLayers{ 3 };
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            // Create a mapping table from object to broad phase layer
            m_objectToBroadPhase[ObjectLayers::NonMoving] = BroadPhaseLayers::NonMoving;
            m_objectToBroadPhase[ObjectLayers::Moving] = BroadPhaseLayers::Moving;
            m_objectToBroadPhase[ObjectLayers::Character] = BroadPhaseLayers::Character;
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
            default:
            {
                JPH_ASSERT(false);
                return "INVALID";
            }
            }
        }
#endif

    private:
        JPH::BroadPhaseLayer m_objectToBroadPhase[ObjectLayers::NumLayers];
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
                return m_playerCollidesWithWorld;
            case ObjectLayers::Moving:
                return true;
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
                return inObject2 == ObjectLayers::Moving; // Non moving only collides with moving
            case ObjectLayers::Character:
                return m_playerCollidesWithWorld;
            case ObjectLayers::Moving:
                return true; // Moving collides with everything
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

    // Callback for traces, connect this to your own trace function if you have one
    void TraceImpl(const char* inFMT, ...);

#ifdef JPH_ENABLE_ASSERTS

    // Callback for asserts, connect this to your own assert handler if you have one
    bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine);

#endif // JPH_ENABLE_ASSERTS

    glm::vec3 JphVec3ToGlm(JPH::Vec3 value);

    glm::quat JphQuatToGlm(JPH::Quat value);

    JPH::Vec3 GlmVec3ToJph(glm::vec3 value);

    JPH::Quat GlmQuatToJph(glm::quat value);

    JPH::Ref<JPH::Shape> CreateBoxShape(const Transform& transform, JPH::Vec3 halfExtents, JPH::Vec3 scale);

    JPH::BodyID CreateBody(JPH::BodyInterface& bodies, JPH::Ref<JPH::Shape> shape, JPH::EMotionType motion, JPH::uint64 userData = 0);
}
