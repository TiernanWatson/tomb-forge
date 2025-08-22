#pragma once

#include <glm/vec3.hpp>

namespace TombForge
{
    struct Plane
    {
        union
        {
            glm::vec3 normal{};
            struct
            {
                float a;
                float b;
                float c;
            };
        };

        float d{};
    };

    void NormalizePlane(Plane& plane);

    float PlanePoint(const Plane& plane, glm::vec3 point);
}
