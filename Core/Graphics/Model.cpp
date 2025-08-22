#include "Model.h"

#include <fstream>

#include "Material.h"

namespace TombForge
{
    // Save Functions

    bool Model::SaveBinary() const
    {
        const std::string& filePath = name;

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            const size_t numMeshes = meshes.size();
            outFile.write((const char*)&numMeshes, sizeof(size_t));

            for (size_t i = 0; i < numMeshes; i++)
            {
                const Mesh& mesh = meshes[i];

                const size_t nameSize = mesh.name.size();
                outFile.write((const char*)&nameSize, sizeof(size_t));
                outFile.write(mesh.name.c_str(), sizeof(char) * nameSize);

                const size_t numVertices = mesh.vertices.size();
                outFile.write((const char*)&numVertices, sizeof(size_t));
                outFile.write((const char*)mesh.vertices.data(), numVertices * sizeof(decltype(mesh.vertices)::value_type));

                const size_t numIndices = mesh.indices.size();
                outFile.write((const char*)&numIndices, sizeof(size_t));
                outFile.write((const char*)mesh.indices.data(), numIndices * sizeof(uint32_t));

                const bool hasMaterial = mesh.material != nullptr;
                outFile.write((const char*)&hasMaterial, sizeof(bool));

                if (hasMaterial)
                {
                    const size_t stringSize = mesh.material->name.size();
                    outFile.write((const char*)&stringSize, sizeof(size_t));
                    outFile.write(mesh.material->name.c_str(), mesh.material->name.size());
                }
            }

            const bool hasSkeleton = skeleton != nullptr;
            outFile.write((const char*)&hasSkeleton, sizeof(bool));
            
            if (hasSkeleton)
            {
                const size_t stringSize = skeleton->name.size();
                outFile.write((const char*)&stringSize, sizeof(size_t));
                outFile.write(skeleton->name.c_str(), sizeof(char) * stringSize);
            }

            return true;
        }

        return false;
    }

    void CalculateBoundingBox(Mesh& mesh)
    {;
        float largestXlocal{ -FLT_MAX };
        float largestYlocal{ -FLT_MAX };
        float largestZlocal{ -FLT_MAX };
        float smallestXlocal{ FLT_MAX };
        float smallestYlocal{ FLT_MAX };
        float smallestZlocal{ FLT_MAX };
        for (auto& v : mesh.vertices)
        {
            if (v.position.x > largestXlocal)
            {
                largestXlocal = v.position.x;
            }

            if (v.position.x < smallestXlocal)
            {
                smallestXlocal = v.position.x;
            }

            if (v.position.y > largestYlocal)
            {
                largestYlocal = v.position.y;
            }

            if (v.position.y < smallestYlocal)
            {
                smallestYlocal = v.position.y;
            }

            if (v.position.z > largestZlocal)
            {
                largestZlocal = v.position.z;
            }

            if (v.position.z < smallestZlocal)
            {
                smallestZlocal = v.position.z;
            }
        }

        mesh.bounds.max = { largestXlocal, largestYlocal, largestZlocal };
        mesh.bounds.min = { smallestXlocal, smallestYlocal, smallestZlocal };
    }

    void CalculateBoundingBox(Model& model)
    {
        model.bounds.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        model.bounds.min = { FLT_MAX, FLT_MAX, FLT_MAX };

        for (auto& mesh : model.meshes)
        {
            AABBJoin(model.bounds, mesh.bounds);
        }
    }

    void MakeUnitPlane(Model& model)
    {
        Mesh& mesh = model.meshes.emplace_back();

        Vertex& v1 = mesh.vertices.emplace_back();
        v1.position = { -0.5f, 0.0f, -0.5f };

        Vertex& v2 = mesh.vertices.emplace_back();
        v2.position = { 0.5f, 0.0f, -0.5f };

        Vertex& v3 = mesh.vertices.emplace_back();
        v3.position = { 0.5f, 0.0f, 0.5f };

        Vertex& v4 = mesh.vertices.emplace_back();
        v4.position = { -0.5f, 0.0f, 0.5f };

        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(1);

        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(3);
    }

    void MakeUnitCube(Model& model)
    {
        Mesh& mesh = model.meshes.emplace_back();

        glm::vec3 p0{ -0.5f, 0.5f, -0.5f }; // Top-Back-Left
        glm::vec3 p1{ 0.5f, 0.5f, -0.5f }; // Top-Back-Right
        glm::vec3 p2{ 0.5f, 0.5f, 0.5f }; // Top-Front-Right
        glm::vec3 p3{ -0.5f, 0.5f, 0.5f }; // Top-Front-Left
        glm::vec3 p4{ -0.5f, -0.5f, -0.5f }; // Bottom-Back-Left
        glm::vec3 p5{ 0.5f, -0.5f, -0.5f }; // Bottom-Back-Right
        glm::vec3 p6{ 0.5f, -0.5f, 0.5f }; // Bottom-Front-Right
        glm::vec3 p7{ -0.5f, -0.5f, 0.5f }; // Bottom-Front-Left

        glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

        AddFlatSurface(mesh, p0, p1, p3, p2, {0.0f, 1.0f, 0.0f}, color); // Top
        AddFlatSurface(mesh, p7, p6, p4, p5, {0.0f, -1.0f, 0.0f}, color); // Bottom
        AddFlatSurface(mesh, p0, p3, p4, p7, {-1.0f, 0.0f, 0.0f}, color); // Left
        AddFlatSurface(mesh, p2, p1, p6, p5, {1.0f, 0.0f, 0.0f}, color); // Right
        AddFlatSurface(mesh, p3, p2, p7, p6, {0.0f, 0.0f, 1.0f}, color); // Front
        AddFlatSurface(mesh, p1, p0, p5, p4, {1.0f, 0.0f, -1.0f}, color); // Back

        CalculateBoundingBox(mesh);
        CalculateBoundingBox(model);
    }

    void MakeUnitCone(Model& model, int segments)
    {
        auto& mesh = model.meshes.emplace_back();
        mesh.vertices.reserve(5);

        auto& v0 = mesh.vertices.emplace_back();
        v0.position = { -0.5f, 0.0f, -0.5f }; // back left

        auto& v1 = mesh.vertices.emplace_back();
        v1.position = { 0.5f, 0.0f, -0.5f }; // back right

        auto& v2 = mesh.vertices.emplace_back();
        v2.position = { 0.5f, 0.0f, 0.5f }; // front right

        auto& v3 = mesh.vertices.emplace_back();
        v3.position = { -0.5f, 0.0f, 0.5f }; // front left

        auto& v4 = mesh.vertices.emplace_back();
        v4.position = { 0.0f, 1.0f, 0.0f }; // tip

        mesh.indices.reserve(18);

        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(1);
        mesh.indices.emplace_back(2);

        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(3);

        mesh.indices.emplace_back(1);
        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(4);

        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(1);
        mesh.indices.emplace_back(4);

        mesh.indices.emplace_back(3);
        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(4);

        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(3);
        mesh.indices.emplace_back(4);

        CalculateBoundingBox(mesh);
        CalculateBoundingBox(model);
    }

    void MakeUnitArrow(Model& model)
    {
        auto& mesh = model.meshes.emplace_back();
        mesh.vertices.reserve(5);

        const float boundary = 0.6f;
        const float tip = 1.0f;
        const float lineHalfWidth = 0.1f;
        const float coneHalfWidth = 0.5f;

        // The cone bit
        auto& v0 = mesh.vertices.emplace_back();
        v0.position = { -coneHalfWidth, -coneHalfWidth, boundary }; // back left

        auto& v1 = mesh.vertices.emplace_back();
        v1.position = { coneHalfWidth, -coneHalfWidth, boundary }; // back right

        auto& v2 = mesh.vertices.emplace_back();
        v2.position = { coneHalfWidth, coneHalfWidth, boundary }; // front right

        auto& v3 = mesh.vertices.emplace_back();
        v3.position = { -coneHalfWidth, coneHalfWidth, boundary }; // front left

        auto& v4 = mesh.vertices.emplace_back();
        v4.position = { 0.0f, 0.0f, tip }; // tip

        mesh.indices.reserve(42);

        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(1);
        mesh.indices.emplace_back(2);

        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(3);

        mesh.indices.emplace_back(1);
        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(4);

        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(1);
        mesh.indices.emplace_back(4);

        mesh.indices.emplace_back(3);
        mesh.indices.emplace_back(2);
        mesh.indices.emplace_back(4);

        mesh.indices.emplace_back(0);
        mesh.indices.emplace_back(3);
        mesh.indices.emplace_back(4);

        // The arrow bit
        auto& v5 = mesh.vertices.emplace_back();
        v5.position = { -lineHalfWidth, -lineHalfWidth, 0.0f }; // back top left

        auto& v6 = mesh.vertices.emplace_back();
        v6.position = { lineHalfWidth, -lineHalfWidth, 0.0f }; // back top right

        auto& v7 = mesh.vertices.emplace_back();
        v7.position = { lineHalfWidth, lineHalfWidth, 0.0f }; // back bottom right

        auto& v8 = mesh.vertices.emplace_back();
        v8.position = { -lineHalfWidth, lineHalfWidth, 0.0f }; // back bottom left

        auto& v9 = mesh.vertices.emplace_back();
        v9.position = { -lineHalfWidth, -lineHalfWidth, boundary }; // front top left

        auto& v10 = mesh.vertices.emplace_back();
        v10.position = { lineHalfWidth, -lineHalfWidth, boundary }; // front top right

        auto& v11 = mesh.vertices.emplace_back();
        v11.position = { lineHalfWidth, lineHalfWidth, boundary }; // front bottom right

        auto& v12 = mesh.vertices.emplace_back();
        v12.position = { -lineHalfWidth, lineHalfWidth, boundary }; // front bottom left

        // top
        mesh.indices.emplace_back(5);
        mesh.indices.emplace_back(6);
        mesh.indices.emplace_back(9);

        mesh.indices.emplace_back(9);
        mesh.indices.emplace_back(6);
        mesh.indices.emplace_back(10);

        // right
        mesh.indices.emplace_back(6);
        mesh.indices.emplace_back(7);
        mesh.indices.emplace_back(10);

        mesh.indices.emplace_back(10);
        mesh.indices.emplace_back(7);
        mesh.indices.emplace_back(11);

        // bottom
        mesh.indices.emplace_back(7);
        mesh.indices.emplace_back(8);
        mesh.indices.emplace_back(11);

        mesh.indices.emplace_back(11);
        mesh.indices.emplace_back(8);
        mesh.indices.emplace_back(12);

        // left
        mesh.indices.emplace_back(9);
        mesh.indices.emplace_back(12);
        mesh.indices.emplace_back(5);

        mesh.indices.emplace_back(5);
        mesh.indices.emplace_back(12);
        mesh.indices.emplace_back(8);

        CalculateBoundingBox(mesh);
        CalculateBoundingBox(model);
    }

    void AddFlatSurface(Mesh& mesh, const glm::vec3& topLeft, const glm::vec3& topRight, const glm::vec3& bottomLeft, const glm::vec3& bottomRight, const glm::vec3& normal,
        const glm::vec4& color)
    {
        Vertex topLeftV{};
        topLeftV.position = topLeft;
        topLeftV.normal = normal;
        topLeftV.color = color;
        topLeftV.uv = {};

        Vertex topRightV{};
        topRightV.position = topRight;
        topRightV.normal = normal;
        topRightV.color = color;
        topRightV.uv = { 1.0f, 0.0f };

        Vertex bottomLeftV{};
        bottomLeftV.position = bottomLeft;
        bottomLeftV.normal = normal;
        bottomLeftV.color = color;
        bottomLeftV.uv = { 0.0f, 1.0f };

        Vertex bottomRightV{};
        bottomRightV.position = bottomRight;
        bottomRightV.normal = normal;
        bottomRightV.color = color;
        bottomRightV.uv = { 1.0f, 1.0f };

        AddFlatSurface(mesh, topLeftV, topRightV, bottomLeftV, bottomRightV);
    }

    void AddFlatSurface(Mesh& mesh, const Vertex& topLeft, const Vertex& topRight, const Vertex& bottomLeft, const Vertex& bottomRight)
    {
        const uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());

        mesh.vertices.emplace_back(topLeft);
        mesh.vertices.emplace_back(bottomRight);
        mesh.vertices.emplace_back(topRight);
        mesh.vertices.emplace_back(bottomLeft);

        mesh.indices.emplace_back(startIndex);
        mesh.indices.emplace_back(startIndex + 1);
        mesh.indices.emplace_back(startIndex + 2);

        mesh.indices.emplace_back(startIndex);
        mesh.indices.emplace_back(startIndex + 3);
        mesh.indices.emplace_back(startIndex + 1);
    }
}
