#include "PhysicsInterface.h"

#include "Physics.h"
#include "PlayerFilters.h"

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace TombForge
{
    bool PhysicsInterface::Raycast(const Ray& ray, HitResult& outResult, PhysicsLayers layers)
    {
        JPH::SpecifiedBroadPhaseLayerFilter bpLayerFilter(BroadPhaseLayers::NonMoving);
        JPH::SpecifiedObjectLayerFilter objLayerFilter(ObjectLayers::NonMoving);

        JPH::RRayCast jphRay{ GlmVec3ToJph(ray.origin), GlmVec3ToJph(ray.direction) };
        JPH::RayCastResult result{};

        if (m_physicsSystem->GetNarrowPhaseQuery().CastRay(jphRay, result, bpLayerFilter, objLayerFilter))
        {
            auto& bodies = m_physicsSystem->GetBodyInterface();

            if (result.mFraction == 0)
            {
                // Colliding inside the object undesirable
                return false;
            }

            const JPH::Vec3 hitPoint = jphRay.GetPointOnRay(result.mFraction);

            outResult.objectId = bodies.GetUserData(result.mBodyID);
            outResult.point = JphVec3ToGlm(hitPoint);

            JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), result.mBodyID);
            if (lock.Succeeded())
            {
                const JPH::Body& body = lock.GetBody();

                outResult.normal = JphVec3ToGlm(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint));
            }

            return true;
        }

        return false;
    }

    void PhysicsInterface::SetPlayerCollidesWorld(bool value)
    {
        m_objBpLayerFilter->SetPlayerCollides(value);
        m_objLayerPairFilter->SetPlayerCollides(value);
        m_playerBpFilter->SetCanCollide(value);
        m_playerObjFilter->SetCanCollide(value);
    }
}
