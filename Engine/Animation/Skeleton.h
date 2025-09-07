#pragma once

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

#include "Engine/Assets/AssetId.h"

namespace TombForge
{
    struct Bone
    {
        std::string name{}; // Human-friendly name

        glm::mat4 offset{}; // Transforms model space vertices to bone space

        glm::mat4 transform{}; // Transform of the bone relative to parent (in bind pose)

        uint8_t parent{}; // Parent index in bones array
    };

    struct Skeleton : public AssetBase
    {
        std::vector<Bone> bones{};

        uint8_t FindBoneId(const std::string& boneName) const;
    };
}

