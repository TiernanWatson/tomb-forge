#pragma once

#include <glm/glm.hpp>

namespace TombForge
{
    bool RayIntersectsTriangle(
        const glm::vec3& rayOrigin, 
        const glm::vec3& rayDir,
        const glm::vec3& v0, 
        const glm::vec3& v1, 
        const glm::vec3& v2,
        float* outT = nullptr);
}
