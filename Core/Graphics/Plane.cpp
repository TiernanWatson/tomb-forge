#include "Plane.h"

#include <cmath>

namespace TombForge
{
    void NormalizePlane(Plane& plane)
    {
        const float sqrMag = plane.a * plane.a + plane.b * plane.b + plane.c * plane.c;
        const float mag = sqrt(sqrMag);

        plane.a /= mag;
        plane.b /= mag;
        plane.c /= mag;
        plane.d /= mag;
    }

    float PlanePoint(const Plane& plane, glm::vec3 point)
    {
        return point.x * plane.a + point.y * plane.b + point.z * plane.c + plane.d;
    }
}
