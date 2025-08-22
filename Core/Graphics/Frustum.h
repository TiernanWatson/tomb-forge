#pragma once

#include "Plane.h"
#include "AABB.h"

namespace TombForge
{
    struct Frustum
    {
        union
        {
            // Normals assumed to point into frustum
            Plane planes[6]{};
            struct
            {
                Plane top;
                Plane left;
                Plane right;
                Plane bottom;
                Plane near;
                Plane far;
            };
        };
    };

    bool FrustumIntersectsAABB(const Frustum& frustum, const AABB& aabb);
}