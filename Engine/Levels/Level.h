#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include "../../Core/Maths/Transform.h"
#include "../../Core/Graphics/Model.h"

#include <functional>
#include <array>

struct GLFWwindow;

namespace TombForge
{
	static constexpr int MaxLightsPerMesh{ 8 };

	using MeshLightArray = std::array<uint32_t, MaxLightsPerMesh>;

	using ColliderId = uint32_t;

	enum ObjectType : uint8_t
	{
		LEVEL_OBJECT_STATIC,
		LEVEL_OBJECT_SKINNED,
		LEVEL_OBJECT_COLLIDER,
		LEVEL_OBJECT_TRIGGER
	};

	enum ColliderType : uint8_t
	{
		COLLIDER_BOX,
		COLLIDER_MESH
	};

	struct Camera
	{
		Transform transform{};

		float fovY{ 45.0f };

		float aspect{ 1024 / 768.0f };

		float near{ 0.1f };

		float far{ 1000.0f };
	};

	struct PointLight
	{
		glm::vec3 position{};

		glm::vec3 color{};

		float innerRadius{};

		float outerRadius{};

		float intensity{ 1.0f };
	};

	struct DirectionalLight
	{
		glm::vec3 color{ 1.0f, 0.95f, 0.9f };

		glm::vec3 dir{ 0.25f, -0.5f, 0.25f };

		float intensity{ 1.0f };
	};

	struct SpotLight
	{
		glm::quat rotation{};

		glm::vec3 position{};

		float angle{ 45.0f };
	};

	struct BoxCollider
	{
		Transform transform{};

		glm::vec3 halfExtents{};

		JPH::BodyID rigidbody{};
	};

	struct MeshCollider
	{
		Transform transform{};

		std::vector<glm::vec3> vertices{};

		std::vector<uint32_t> indices{};

		JPH::BodyID rigidbody{};
	};

	struct ColliderInfo
	{
		ColliderType type{}; // Which array to look at

		ColliderId id{}; // Index into specific array
	};

	struct LevelObject
	{
		std::string	name{};

		std::shared_ptr<Model> model{};

		uint32_t meshIndex{};
		
		JPH::BodyID rigidbody{}; // Invalid ID if empty

		glm::vec3 halfExtents{};

		Transform transform{};

		std::vector<MeshLightArray> lights{};

		AABB bounds{};

		uint32_t parent{};

		bool visible{};
	};

	struct LedgePoint
	{
		glm::vec3 point{};

		glm::vec3 direction{};

		JPH::BodyID bodyId{};

		uint32_t nextLedge{};
	};

	struct MeshInstance
	{
		std::string name{};

		Transform transform{};

		glm::mat4 modelMatrix{}; // World-space, only updated if moved

		Mesh* mesh{};

		AABB bounds{};

		MeshLightArray lights{};

		ColliderInfo collision{};
	};

	struct Level
	{
		std::string name{};

		// Objects
		std::vector<std::shared_ptr<Model>> models{};
		std::vector<LevelObject> staticObjects{};
		std::vector<MeshInstance> meshes{};

		// Colliders
		std::vector<BoxCollider> boxColliders{};
		std::vector<MeshCollider> meshColliders{};

		// Triggers
		std::vector<LedgePoint> ledges{};

		// Lights
		std::vector<PointLight> pointLights{};
		std::vector<SpotLight> spotLights{};
		DirectionalLight directionalLight{};

		// Player
		glm::vec3 startPosition{};

		// Ambient
		glm::vec3 ambientColor{ 1.0f, 1.0f, 1.0f };
		float ambientStrength{ 1.0f };

		bool Save();
	};

	void UpdateBounds(LevelObject& obj);

	void UpdateBounds(MeshInstance& meshInstance);

	AABB CalculateLevelBounds(const Level& level);

	void GetClosestLights(const Level& level, const glm::vec3& position, MeshLightArray& result);

	void InitializeCollider(MeshInstance& mesh, const glm::vec3& extents);
}

