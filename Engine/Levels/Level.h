#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsScene.h>

#include "Core/Maths/Transform.h"
#include "Engine/Assets/AssetId.h"
#include "Engine/Audio/Sound.h"
#include "Engine/Rendering/Model.h"

// These Win32 macros conflict with camera code
#undef near
#undef far

struct GLFWwindow;

namespace TombForge
{
    static constexpr int MaxLightsPerMesh{ 8 };

    using MeshLightArray = std::array<uint32_t, MaxLightsPerMesh>;
    using ColliderId = uint32_t;
    using ModelId = uint32_t;
    using MeshId = uint32_t;

    enum ColliderType : uint8_t
    {
        COLLIDER_NONE,
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
        glm::vec3 color{}; // Linear space
        float innerRadius{};
        float outerRadius{};
        float intensity{};
    };

    struct DirectionalLight
    {
        glm::vec3 color{}; // Linear space
        glm::vec3 dir{};
        float intensity{};
    };

    struct SpotLight
    {
        glm::quat rotation{};
        glm::vec3 position{};
        float angle{};
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

    struct Collision
    {
        ColliderType type{}; // Which array to look at
        ColliderId id{}; // Index into specific array
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
        std::shared_ptr<Material> overrideMaterial{}; // If set, overrides the mesh material
        Transform transform{};
        glm::mat4 modelMatrix{}; // World-space, only updated if moved
        MeshLightArray lights{};
        uint32_t model{}; // Index into model array
        uint32_t mesh{}; // Index into model's mesh array
        AABB bounds{}; // World-space, only updated if moved
        Collision collision{};
        uint8_t lightCount{};
    };

    struct SoundInstance
    {
        std::shared_ptr<Sound> sound{};
        glm::vec3 position{};
        float volume{ 1.0f };
        float radius{}; // Max distance for sound to be heard
        bool loop{ true };
    };

    struct Level : public AssetBase
    {
        // Objects
        std::vector<std::shared_ptr<Model>> models{};
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

        // Ambient
        glm::vec3 ambientColor{ 1.0f, 1.0f, 1.0f };
        float ambientStrength{ 1.0f };

        // Audio
        std::vector<SoundInstance> sounds{};
        std::shared_ptr<Sound> ambientSound{};
        float ambientSoundVolume{ 1.0f };

        // Player
        glm::vec3 startPosition{};
    };

    void UpdateBounds(const Level& level, MeshInstance& meshInstance);
    AABB CalculateLevelBounds(const Level& level);
    void GetClosestLights(const Level& level, const glm::vec3& position, MeshLightArray& result, uint8_t& lightCount);
    void UpdateAllClosestLights(Level& level);
    void InitializeCollider(MeshInstance& mesh, const glm::vec3& extents);
}
