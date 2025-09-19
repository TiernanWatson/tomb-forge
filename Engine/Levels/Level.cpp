#include "Engine/Levels/Level.h"

#include <nlohmann/json.hpp>
#include <fstream>

#include "Engine/Physics/Physics.h"

namespace TombForge
{
    void UpdateBounds(const Level& level, MeshInstance& meshInstance)
    {
        const glm::mat4 transform = meshInstance.transform.AsMatrix();

        auto& mesh = level.models[meshInstance.model]->meshes[meshInstance.mesh];
        meshInstance.bounds = mesh.bounds;
        meshInstance.bounds.max = transform * glm::vec4(meshInstance.bounds.max, 1.0f);
        meshInstance.bounds.min = transform * glm::vec4(meshInstance.bounds.min, 1.0f);
    }

    AABB CalculateLevelBounds(const Level& level)
    {
        if (level.meshes.size() > 0)
        {
            glm::vec3 min{ FLT_MAX, FLT_MAX, FLT_MAX };
            glm::vec3 max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

            for (const auto& obj : level.meshes)
            {
                min.x = obj.bounds.min.x < min.x ? obj.bounds.min.x : min.x;
                min.y = obj.bounds.min.y < min.y ? obj.bounds.min.y : min.y;
                min.z = obj.bounds.min.z < min.z ? obj.bounds.min.z : min.z;

                max.x = obj.bounds.max.x > max.x ? obj.bounds.max.x : max.x;
                max.y = obj.bounds.max.y > max.y ? obj.bounds.max.y : max.y;
                max.z = obj.bounds.max.z > max.z ? obj.bounds.max.z : max.z;
            }

            return { max, min };
        }
        else
        {
            return {};
        }
    }

    void GetClosestLights(const Level& level, const glm::vec3& position, MeshLightArray& result)
    {
        // todo: There are problems with this algorithm in AOD levels, as they are room based.
        // Look at taking occlusion into account using ray cast. If this doesn't help, need to look
        // at sorting them as they come in from the level file and use the room name to compare.

        std::vector<uint32_t> lightIndices{};
        lightIndices.resize(level.pointLights.size());
        for (size_t i = 0; i < level.pointLights.size(); i++)
        {
            lightIndices[i] = static_cast<uint32_t>(i);
        }

        std::sort(lightIndices.begin(), lightIndices.end(),
            [&](uint32_t i1, uint32_t i2)
            {
                float dist1 = glm::length(level.pointLights[i1].position - position);
                float dist2 = glm::length(level.pointLights[i2].position - position);
                return dist1 < dist2;
            });

        const size_t copyCount = lightIndices.size() < MaxLightsPerMesh ? lightIndices.size() : MaxLightsPerMesh;
        memcpy(result.data(), lightIndices.data(), copyCount * sizeof(uint32_t));
    }

    void UpdateAllClosestLights(Level& level)
    {
        for (auto& mesh : level.meshes)
        {
            const glm::vec3 lightReferencePosition = (mesh.bounds.min + mesh.bounds.max) / 2.0f;
            GetClosestLights(level, lightReferencePosition, mesh.lights);
        }
    }

    void InitializeCollider(MeshInstance& mesh, const glm::vec3& extents)
    {
        
    }
}
