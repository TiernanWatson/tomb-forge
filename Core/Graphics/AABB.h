#pragma once

#include <glm/vec3.hpp>

namespace TombForge
{
    struct AABB
    {
        glm::vec3 max{};

        glm::vec3 min{};
    };

    // Clamps bounds values within a range
    void ClampBounds(AABB& bounds, const glm::vec3& min, const glm::vec3& max);

    // Checks whether an AABB is fully contained (not just intersecting) in another
    bool AABBIsContained(const AABB& container, const AABB& containee);

    // Checks whether two AABBs are intersecting
    bool AABBIntersect(const AABB& one, const AABB& two);

    // Expands an AABB by joining it with another
    void AABBJoin(AABB& result, const AABB& swallow);
}
