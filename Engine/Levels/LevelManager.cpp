#include "LevelManager.h"

#include <assert.h>
#include <fstream>
#include <nlohmann/json.hpp>

#include "../Physics/Physics.h"

namespace TombForge
{
	// Level functions

	std::shared_ptr<Level> LevelManager::Read(const std::string& name)
	{
		std::ifstream inFile(name);

		if (inFile.is_open())
		{
			std::shared_ptr<Level> result = std::make_shared<Level>();
			result->name = name;

			nlohmann::json json = nlohmann::json::parse(inFile);

			JPH::uint64 currentIndex{};
			for (auto& obj : json["statics"])
			{
				if (m_modelLoader)
				{
					auto model = m_modelLoader->Load(obj["model"]);
					result->models.emplace_back(model);
					for (auto& mesh : model->meshes)
					{
						MeshInstance& instance = result->meshes.emplace_back();
						instance.name = obj["name"];

						instance.transform.position.x = obj["transform"]["position"]["x"];
						instance.transform.position.y = obj["transform"]["position"]["y"];
						instance.transform.position.z = obj["transform"]["position"]["z"];

						instance.transform.SetEulers(
							obj["transform"]["rotation"]["x"],
							obj["transform"]["rotation"]["y"],
							obj["transform"]["rotation"]["z"]);

						instance.transform.scale.x = obj["transform"]["scale"]["x"];
						instance.transform.scale.y = obj["transform"]["scale"]["y"];
						instance.transform.scale.z = obj["transform"]["scale"]["z"];

						instance.modelMatrix = instance.transform.AsMatrix();

						instance.mesh = &mesh;

						UpdateBounds(instance);

						bool hasPhysics = obj["physics"] == 1;
						if (hasPhysics && m_physicsSystem)
						{
							glm::vec3 halfExtents{};
							halfExtents.x = obj["halfExtents"]["x"];
							halfExtents.y = obj["halfExtents"]["y"];
							halfExtents.z = obj["halfExtents"]["z"];

							auto& boxCollider = result->boxColliders.emplace_back(BoxCollider{ instance.transform, halfExtents, {} });

							JPH::Vec3 halfExtents2 = GlmVec3ToJph(halfExtents);
							JPH::Vec3 scale = GlmVec3ToJph(instance.transform.scale);
							JPH::Ref<JPH::Shape> shape = CreateBoxShape(instance.transform, halfExtents2, scale);

							auto& bodies = m_physicsSystem->GetBodyInterface();
							boxCollider.rigidbody = CreateBody(bodies, shape, JPH::EMotionType::Static, result->meshes.size() - 1);

							instance.collision.type = COLLIDER_BOX;
							instance.collision.id = static_cast<uint32_t>(result->boxColliders.size() - 1);

							ASSERT(instance.collision.id < result->boxColliders.size(), "Exceed bounds of uint32_t number of colliders");
						}
					}
					/*instance.model = m_modelLoader->Load(obj["model"]);
					if (instance.model)
					{
						UpdateBounds(instance);
					}*/
				}

				/*instance.halfExtents.x = obj["halfExtents"]["x"];
				instance.halfExtents.y = obj["halfExtents"]["y"];
				instance.halfExtents.z = obj["halfExtents"]["z"];

				bool hasPhysics = obj["physics"] == 1;
				if (hasPhysics && m_physicsSystem)
				{
					JPH::Vec3 halfExtents = GlmVec3ToJph(instance.halfExtents);
					JPH::Vec3 scale = GlmVec3ToJph(instance.transform.scale);
					JPH::Ref<JPH::Shape> shape = CreateBoxShape(instance.transform, halfExtents, scale);

					auto& bodies = m_physicsSystem->GetBodyInterface();
					instance.rigidbody = CreateBody(bodies, shape, JPH::EMotionType::Static, currentIndex++);
				}

				instance.lights.resize(instance.model->meshes.size());*/
			}

			return result;
		}

		return nullptr;
	}
}
