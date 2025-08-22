#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace TombForge
{
    struct Bone
    {
        std::string name{}; // Human-friendly name

        glm::mat4 offset{}; // Transforms model space vertices to bone space

        glm::mat4 transform{}; // Transform of the bone relative to parent (in bind pose)

        uint8_t parent{}; // Parent index in bones array
    };

    struct Skeleton
    {
        std::string name{};

        std::vector<Bone> bones{};

        uint8_t FindBoneId(const std::string& boneName) const;

        bool Load();

        bool SaveBinary() const;
    };
}

