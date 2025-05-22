#include "AABB.h"

#include <cmath>
#include "../Maths/Maths.h"

namespace TombForge
{
    __forceinline constexpr bool CheckOverlap(float oneMin, float oneMax, float twoMin, float twoMax)
    {
        return oneMin < twoMax && oneMax > twoMin;
    }

    void ClampBounds(AABB& bounds, const glm::vec3& min, const glm::vec3& max)
    {
        bounds.min.x = Maths::Clamp(bounds.min.x, min.x, max.x);
        bounds.min.y = Maths::Clamp(bounds.min.y, min.y, max.y);
        bounds.min.z = Maths::Clamp(bounds.min.z, min.z, max.z);

        bounds.max.x = Maths::Clamp(bounds.max.x, min.x, max.x);
        bounds.max.y = Maths::Clamp(bounds.max.y, min.y, max.y);
        bounds.max.z = Maths::Clamp(bounds.max.z, min.z, max.z);
    }

    bool AABBIsContained(const AABB& container, const AABB& containee)
    {
        return container.max.x > containee.max.x
            && container.max.y > containee.max.y
            && container.max.z > containee.max.z
            && container.min.x < containee.min.x
            && container.min.y < containee.min.y
            && container.min.z < containee.min.z;
    }

    bool AABBIntersect(const AABB& one, const AABB& two)
    {
        return CheckOverlap(one.min.x, one.max.x, two.min.x, two.max.x)
            && CheckOverlap(one.min.y, one.max.y, two.min.y, two.max.y)
            && CheckOverlap(one.min.z, one.max.z, two.min.z, two.max.z);
    }

    void AABBJoin(AABB& result, const AABB& swallow)
    {
        result.max.x = Maths::Max(result.max.x, swallow.max.x);
        result.max.y = Maths::Max(result.max.y, swallow.max.y);
        result.max.z = Maths::Max(result.max.z, swallow.max.z);

        result.min.x = Maths::Min(result.min.x, swallow.min.x);
        result.min.y = Maths::Min(result.min.y, swallow.min.y);
        result.min.z = Maths::Min(result.min.z, swallow.min.z);
    }
}
