#include "Physics.h"

#include <cstdarg>

namespace TombForge
{
    // Callback for traces, connect this to your own trace function if you have one
    void TraceImpl(const char* inFMT, ...)
    {
        // Format the message
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);
    }

#ifdef JPH_ENABLE_ASSERTS

    // Callback for asserts, connect this to your own assert handler if you have one
    bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
    {
        // Breakpoint
        return true;
    };

#endif // JPH_ENABLE_ASSERTS

    glm::vec3 JphVec3ToGlm(JPH::Vec3 value)
    {
        return glm::vec3{ value.GetX(), value.GetY(), value.GetZ() };
    }

    glm::quat JphQuatToGlm(JPH::Quat value)
    {
        return glm::quat{ value.GetW(), value.GetX(), value.GetY(), value.GetZ() };
    }

    JPH::Vec3 GlmVec3ToJph(glm::vec3 value)
    {
        return JPH::Vec3{ value.x, value.y, value.z };
    }

    JPH::Quat GlmQuatToJph(glm::quat value)
    {
        return JPH::Quat{ value.x, value.y, value.z, value.w };
    }

    JPH::Ref<JPH::Shape> CreateBoxShape(const Transform& transform, JPH::Vec3 halfExtents, JPH::Vec3 scale)
    {
        return new JPH::RotatedTranslatedShape(
            GlmVec3ToJph(transform.position),
            GlmQuatToJph(transform.rotation),
            new JPH::BoxShape(halfExtents * scale));
    }

    JPH::BodyID CreateBody(JPH::BodyInterface& bodies, JPH::Ref<JPH::Shape> shape, JPH::EMotionType motion, JPH::uint64 userData)
    {
        JPH::BodyCreationSettings settings{};
        settings.mMotionType = motion;
        settings.SetShape(shape);
        settings.mUserData = userData;

        return bodies.CreateAndAddBody(settings, JPH::EActivation::Activate);
    }
}