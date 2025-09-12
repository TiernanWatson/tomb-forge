#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

#include "Engine/Physics/PhysicsLayers.h"
#include "Engine/Physics/PlayerFilters.h"

namespace JPH
{
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystem;
    class BodyInterface;
}

namespace TombForge
{
    struct PhysicsContext
    {
        JPH::PhysicsSystem* system{};
        JPH::TempAllocatorImpl* tmpAllocator{};
        JPH::JobSystem* jobSystem{};

        BPLayerInterfaceImpl bpLayerInterface{};
        ObjectVsBroadPhaseLayerFilterImpl objVsBpLayerFilter{};
        ObjectLayerPairFilterImpl objVsObjLayerFilter{};
        PlayerBpFilter playerBpFilter{};
        PlayerLayerFilter playerLayerFilter{};
    };

    bool InitPhysics(PhysicsContext& ctx);
    void DestroyPhysics(PhysicsContext& ctx);

    inline glm::vec3 JphVec3ToGlm(JPH::Vec3 value)
    {
        return glm::vec3{ value.GetX(), value.GetY(), value.GetZ() };
    }

    inline glm::quat JphQuatToGlm(JPH::Quat value)
    {
        return glm::quat{ value.GetW(), value.GetX(), value.GetY(), value.GetZ() };
    }

    inline JPH::Vec3 GlmVec3ToJph(glm::vec3 value)
    {
        return JPH::Vec3{ value.x, value.y, value.z };
    }

    inline JPH::Quat GlmQuatToJph(glm::quat value)
    {
        return JPH::Quat{ value.x, value.y, value.z, value.w };
    }
}
