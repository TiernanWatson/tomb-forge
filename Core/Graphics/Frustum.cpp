#include "Frustum.h"

#include "../Maths/Maths.h"

#include "../Debug.h"

namespace TombForge
{
    bool FrustumIntersectsAABB(const Frustum& frustum, const AABB& aabb)
    {
        const glm::vec3 center = (aabb.max + aabb.min) / 2.0f;
        const glm::vec3 extents = (aabb.max - aabb.min) / 2.0f;

        for (const auto& plane : frustum.planes)
        {
            const float longest = extents.x * Maths::Abs(plane.a)
                + extents.y * Maths::Abs(plane.b)
                + extents.z * Maths::Abs(plane.c);

            ASSERT(longest >= 0, "Longest value for frustum should be positive");

            const float dist = PlanePoint(plane, center);

            if (dist + longest < 0)
                return false; // This means box is behind plane (plane points in)
        }

        return true;
    }
}
