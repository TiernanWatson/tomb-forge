#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Engine/Animation/Skeleton.h"
#include "AABB.h"
#include "Graphics.h"

namespace TombForge
{
    struct Material;

    struct Vertex
    {
        glm::vec3 position{};

        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };

        glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

        glm::vec2 uv{};

        glm::ivec4 boneIndices{ -1, -1, -1, -1 };

        glm::vec4 boneWeights{};
    };

    struct Mesh
    {
        std::string name{};

        std::vector<Vertex> vertices{};

        std::vector<uint32_t> indices{};

        std::shared_ptr<Material> material{};

        AABB bounds{}; // In mesh space

        MeshHandle gpuHandle{}; // Only valid if mesh has been instantiated

        bool isActive{ true };

        bool isDoubleSided{};
    };

    struct Model
    {
        std::string name{};

        std::vector<Mesh> meshes{};

        std::shared_ptr<Skeleton> skeleton{}; // nullptr means its static

        AABB bounds{};

        bool SaveBinary() const;
    };

    void CalculateBoundingBox(Mesh& mesh);

    void CalculateBoundingBox(Model& model);

    // Helper mesh functions

    void MakeUnitPlane(Model& model);

    void MakeUnitCube(Model& model);

    void MakeUnitCone(Model& model, int segments);

    void MakeUnitArrow(Model& model);

    void AddFlatSurface(Mesh& mesh, 
        const glm::vec3& topLeft, 
        const glm::vec3& topRight, 
        const glm::vec3& bottomLeft, 
        const glm::vec3& bottomRight, 
        const glm::vec3& normal,
        const glm::vec4& color);

    void AddFlatSurface(Mesh& mesh, const Vertex& topLeft, const Vertex& topRight, const Vertex& bottomLeft, const Vertex& bottomRight);
}

