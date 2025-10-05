#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <cstdint>

#include "Engine/Assets/AssetId.h"

namespace TombForge
{
    struct CollisionMesh : public AssetBase
    {
        std::vector<glm::vec3> vertices{};
        std::vector<uint32_t> indices{};
    };
}
