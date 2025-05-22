#pragma once

#include <cstdint>
#include <glm/vec3.hpp>

#if DEVSLATE
#include "../Rendering/JoltDebugRenderer.h"
#define DEBUG_RAY(physInterface, ray, color) physInterface.DebugRenderer()->DrawColoredLine(ray.origin, ray.origin + ray.direction, color)
#define DEBUG_RAY(physInterface, ray) physInterface.DebugRenderer()->DrawColoredLine(ray.origin, ray.origin + ray.direction, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f})
#else
#define DEBUG_RAY(physInterface, ray, color)
#define DEBUG_RAY(physInterface, ray)
#endif

namespace JPH
{
	class PhysicsSystem;
}

namespace TombForge
{
	class JoltDebugRenderer;
	class ObjectVsBroadPhaseLayerFilterImpl;
	class ObjectLayerPairFilterImpl;
	class PlayerBpFilter;
	class PlayerLayerFilter;

	struct HitResult
	{
		glm::vec3 point{}; // Point exactly that was hit

		glm::vec3 normal{}; // Surface normal from hit

		uint32_t objectId{}; // Object ID in level array
	};

	struct Ray
	{
		glm::vec3 origin{};

		glm::vec3 direction{}; // Length is the total distance
	};

	enum PhysicsLayers : uint8_t
	{
		PHYSICS_STATIC = 1,
		PHYSICS_MOVING = 2,
		PHYSICS_PLAYER = 4,
		PHYSICS_PLAYER_INTERNALS = 8,
		PHYSICS_TRIGGERS = 16,

		PHYSICS_LAYER_COUNT = 5
	};

	class PhysicsInterface
	{
	public:
		bool Raycast(const Ray& ray, HitResult& outResult, PhysicsLayers layers = PHYSICS_STATIC);

		void SetPlayerCollidesWorld(bool value);

#if DEVSLATE
		inline JoltDebugRenderer* DebugRenderer() const
		{
			return m_debugRenderer;
		}

		inline void SetDebugRenderer(JoltDebugRenderer* value)
		{
			m_debugRenderer = value;
		}
#endif

		inline void SetSystem(JPH::PhysicsSystem* system)
		{
			m_physicsSystem = system;
		}

		inline void SetObjBroadPhaseFilter(ObjectVsBroadPhaseLayerFilterImpl* value)
		{
			m_objBpLayerFilter = value;
		}

		inline void SetObjectLayerPairFilter(ObjectLayerPairFilterImpl* value)
		{
			m_objLayerPairFilter = value;
		}

		inline void SetPlayerBpFilter(PlayerBpFilter* value)
		{
			m_playerBpFilter = value;
		}

		inline void SetPlayerLayerFilter(PlayerLayerFilter* value)
		{
			m_playerObjFilter = value;
		}

	private:
		JPH::PhysicsSystem* m_physicsSystem{};

		ObjectVsBroadPhaseLayerFilterImpl* m_objBpLayerFilter{};
		ObjectLayerPairFilterImpl* m_objLayerPairFilter{};
		PlayerBpFilter* m_playerBpFilter{};
		PlayerLayerFilter* m_playerObjFilter{};

#if DEVSLATE
		JoltDebugRenderer* m_debugRenderer{};
#endif
	};
}

