#include "Level.h"

#include <nlohmann/json.hpp>
#include <fstream>

#include "../Physics/Physics.h"

namespace TombForge
{
	// Level functions

	bool Level::Save()
	{
		std::ofstream outFile(name);

		if (!outFile.is_open())
		{
			return false;
		}

		nlohmann::json json;

		for (size_t i = 0; i < staticObjects.size(); i++)
		{
			auto& obj = staticObjects[i];

			glm::vec3 rotation = obj.transform.EulerRotation();

			auto& jsonSec = json["statics"][i];
			jsonSec["transform"]["position"]["x"] = obj.transform.position.x;
			jsonSec["transform"]["position"]["y"] = obj.transform.position.y;
			jsonSec["transform"]["position"]["z"] = obj.transform.position.z;
			jsonSec["transform"]["rotation"]["x"] = rotation.x;
			jsonSec["transform"]["rotation"]["y"] = rotation.y;
			jsonSec["transform"]["rotation"]["z"] = rotation.z;
			jsonSec["transform"]["scale"]["x"] = obj.transform.scale.x;
			jsonSec["transform"]["scale"]["y"] = obj.transform.scale.y;
			jsonSec["transform"]["scale"]["z"] = obj.transform.scale.z;
			json["statics"][i]["name"] = obj.name;
			json["statics"][i]["model"] = obj.model->name;
			json["statics"][i]["parent"] = obj.parent;
			json["statics"][i]["physics"] = obj.rigidbody.IsInvalid() ? 0 : 1;
			json["statics"][i]["halfExtents"]["x"] = obj.halfExtents.x;
			json["statics"][i]["halfExtents"]["y"] = obj.halfExtents.y;
			json["statics"][i]["halfExtents"]["z"] = obj.halfExtents.z;
		}

		outFile << json.dump(4);

		outFile.flush();
		outFile.close();

		return true;
	}

	void UpdateBounds(LevelObject& obj)
	{
		const glm::vec3 corners[8]
		{
			obj.model->bounds.min,
			{ obj.model->bounds.max.x, obj.model->bounds.min.y, obj.model->bounds.min.z },
			{ obj.model->bounds.max.x, obj.model->bounds.min.y, obj.model->bounds.max.z },
			{ obj.model->bounds.min.x, obj.model->bounds.min.y, obj.model->bounds.max.z },

			{ obj.model->bounds.min.x, obj.model->bounds.max.y, obj.model->bounds.min.z },
			{ obj.model->bounds.max.x, obj.model->bounds.max.y, obj.model->bounds.min.z },
			obj.model->bounds.max,
			{ obj.model->bounds.min.x, obj.model->bounds.max.y, obj.model->bounds.max.z },
		};

		const glm::mat4 transform = obj.transform.AsMatrix();

		float minX{ FLT_MAX };
		float minY{ FLT_MAX };
		float minZ{ FLT_MAX };
		float maxX{ -FLT_MAX };
		float maxY{ -FLT_MAX };
		float maxZ{ -FLT_MAX };

		for (const auto& c : corners)
		{
			const glm::vec3 c2 = transform * glm::vec4(c, 1.0f);

			minX = c2.x < minX ? c2.x : minX;
			minY = c2.y < minY ? c2.y : minY;
			minZ = c2.z < minZ ? c2.z : minZ;

			maxX = c2.x > maxX ? c2.x : maxX;
			maxY = c2.y > maxY ? c2.y : maxY;
			maxZ = c2.z > maxZ ? c2.z : maxZ;
		}

		obj.bounds.min = { minX, minY, minZ };
		obj.bounds.max = { maxX, maxY, maxZ };
	}

	void UpdateBounds(MeshInstance& meshInstance)
	{
		if (!meshInstance.mesh)
		{
			return;
		}

		const glm::mat4 transform = meshInstance.transform.AsMatrix();

		meshInstance.bounds = meshInstance.mesh->bounds;

		meshInstance.bounds.max = transform * glm::vec4(meshInstance.bounds.max, 1.0f);
		meshInstance.bounds.min = transform * glm::vec4(meshInstance.bounds.min, 1.0f);
	}

	AABB CalculateLevelBounds(const Level& level)
	{
		if (level.meshes.size() > 1)
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

		const size_t copyCount = lightIndices.size() < 8 ? lightIndices.size() : 8;
		memcpy(result.data(), lightIndices.data(), copyCount * sizeof(uint32_t));
	}

	void InitializeCollider(MeshInstance& mesh, const glm::vec3& extents)
	{
		
	}
}
