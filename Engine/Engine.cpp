#include "Engine/Engine.h"

#include <cmath>

#include <glad/glad.h>
#include <glfw3.h>
#include <glm/glm.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include "Core/Config.h"
#include "Core/Maths/Maths.h"
#include "Engine/Player/Input.h"
#include "Engine/Player/States/AirState.h"
#include "Engine/Player/States/ClimbState.h"
#include "Engine/Player/States/LocomotionState.h"
#include "Engine/Rendering/Material.h"

namespace TombForge
{
    namespace
    {
        constexpr float MaxDeltaTime = 0.1f;

        constexpr float CameraHeight = 1.25f;
        constexpr float CameraDistance = 4.0f;
        constexpr float CameraRetreatRate = 0.15f;
        constexpr float CameraApproachRate = 0.05f;

        constexpr AssetId DebugTextureId = 0;
        constexpr AssetId DebugMaterialId = 1;
        constexpr AssetId CubeId = 2;
        constexpr AssetId ConeId = 3;
        constexpr AssetId ArrowId = 4;

        void HandleMouseMove(GLFWwindow* window, double x, double y)
        {
            const float xf = static_cast<float>(x);
            const float yf = static_cast<float>(y);

            if (EngineContext* ctx = reinterpret_cast<EngineContext*>(glfwGetWindowUserPointer(window)))
            {
                ctx->mouseX = xf;
                ctx->mouseY = yf;
            }

            Input::HandleMouseMove(xf, yf);
        }

        void HandleMouseScroll(GLFWwindow* window, double scrollX, double scrollY)
        {
            Input::HandleMouseScroll(static_cast<float>(scrollY));
        }

        void HandleMouseButton(GLFWwindow* window, int button, int action, int mods)
        {
            Input::HandleMouseButton(button, action, mods);
        }

        void HandleKey(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            Input::HandleKey(key, scancode, action, mods);
        }

        void HandleFramebufferResize(GLFWwindow* window, int width, int height)
        {
            if (EngineContext* ctx = reinterpret_cast<EngineContext*>(glfwGetWindowUserPointer(window)))
            {
                if (std::isnan(width) || std::isnan(height) || height == 0.0f)
                {
                    // This can come from minimizing the window
                    return;
                }

                ctx->windowWidth = width;
                ctx->windowHeight = height;
                ctx->camera.aspect = static_cast<float>(width) / height;
                ctx->renderer.OnWindowResized(width, height);
            }
        }

        void GenerateDebugTexture(Texture& texture, glm::ivec4 background, glm::ivec4 line)
        {
            constexpr uint32_t LineThickness = 10;

            texture.width = texture.height = 1024;
            texture.format = TextureFormat::RGBA;

            texture.data.resize(texture.width * texture.height * 4);

            for (uint32_t w = 0; w < texture.width; w++)
            {
                for (uint32_t h = 0; h < texture.height; h++)
                {
                    const size_t baseIndex = (w * texture.width * 4) + h * 4;

                    if (w < LineThickness 
                        || h < LineThickness 
                        || w > (texture.width - LineThickness) 
                        || h > (texture.height - LineThickness))
                    {
                        texture.data[baseIndex] = line.x;
                        texture.data[baseIndex + 1] = line.y;
                        texture.data[baseIndex + 2] = line.b;
                        texture.data[baseIndex + 3] = line.a;
                    }
                    else
                    {
                        texture.data[baseIndex] = background.x;
                        texture.data[baseIndex + 1] = background.y;
                        texture.data[baseIndex + 2] = background.b;
                        texture.data[baseIndex + 3] = background.a;
                    }
                }
            }
        }

        void SetupDefaultShapes(EngineContext& ctx)
        {
            // Texture

            std::shared_ptr<Texture> debugTexture = std::make_shared<Texture>();
            GenerateDebugTexture(*debugTexture, glm::ivec4{ 20, 50, 200, 255 }, glm::ivec4{ 20, 20, 20, 255 });
            debugTexture->id = DebugTextureId;
            debugTexture->name = "Debug Texture";
            ctx.assetRegistry.AddAssetBuiltin(debugTexture);

            // Material

            std::shared_ptr<Material> debugMaterial = std::make_shared<Material>();
            debugMaterial->albedoTexture = debugTexture;
            debugMaterial->id = DebugMaterialId;
            debugMaterial->name = "Debug Material";
            ctx.assetRegistry.AddAssetBuiltin(debugMaterial);

            // Cube

            std::shared_ptr<Model> cube = std::make_shared<Model>();
            MakeUnitCube(*cube);
            cube->meshes[0].material = debugMaterial;
            cube->id = CubeId;
            cube->name = "Cube";
            ctx.assetRegistry.AddAssetBuiltin(cube);
            ctx.renderer.InitializeModel(*cube);

            // Cone

            std::shared_ptr<Model> cone = std::make_shared<Model>();
            MakeUnitCone(*cone, 4);
            cone->meshes[0].material = debugMaterial;
            cone->id = ConeId;
            cone->name = "Cone";
            ctx.assetRegistry.AddAssetBuiltin(cone);
            ctx.renderer.InitializeModel(*cone);

            // Arrow

            std::shared_ptr<Model> arrow = std::make_shared<Model>();
            MakeUnitArrow(*arrow);
            arrow->meshes[0].material = debugMaterial;
            arrow->id = ArrowId;
            arrow->name = "Arrow";
            ctx.assetRegistry.AddAssetBuiltin(arrow);
            ctx.renderer.InitializeModel(*arrow);
        }

        JPH::BodyID CreateBody(JPH::BodyInterface& bodies, JPH::Ref<JPH::Shape> shape, JPH::EMotionType motion, JPH::uint64 userData, bool isTrigger = false, JPH::ObjectLayer layer = ObjectLayers::NonMoving)
        {
            JPH::BodyCreationSettings settings{};
            settings.mMotionType = motion;
            settings.SetShape(shape);
            settings.mUserData = userData;
            settings.mIsSensor = isTrigger;
            settings.mObjectLayer = layer;

            return bodies.CreateAndAddBody(settings, JPH::EActivation::Activate);
        }

        void SetupLara(EngineContext& ctx, AssetId assetId)
        {
            if (!IsValidAssetId(assetId))
            {
                LOG_ERROR("Lara ID not valid: %i", assetId);
                return;
            }

            ctx.lara.model = ctx.assetRegistry.Load<Model>(assetId);
            if (!ctx.lara.model)
            {
                LOG_ERROR("Failed to load Lara model with ID: %i", assetId);
                return;
            }

            ctx.lara.animPlayer.SetSkeleton(ctx.lara.model->skeleton);

            ctx.renderer.InitializeModel(*ctx.lara.model);

            constexpr float LaraHeight = 1.75f;
            constexpr float LaraRadius = 0.25f;

            auto shapeSettings = JPH::RotatedTranslatedShapeSettings(
                JPH::Vec3(0, 0.5f * LaraHeight, 0),
                JPH::Quat::sIdentity(),
                new JPH::CapsuleShape(0.5f * (LaraHeight - 2.0f * LaraRadius), LaraRadius)).Create().Get();

            JPH::Ref<JPH::CharacterVirtualSettings> characterSettings = new JPH::CharacterVirtualSettings();
            characterSettings->mShape = shapeSettings;
            characterSettings->mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
            characterSettings->mInnerBodyLayer = ObjectLayers::Character;
            characterSettings->mSupportingVolume = JPH::Plane{ JPH::Vec3::sAxisY(), -LaraRadius };

            ctx.lara.physics = new JPH::CharacterVirtual(characterSettings, JPH::Vec3::sZero(), JPH::Quat::sIdentity(), 0, ctx.physics.system);
        }

        JPH::Ref<JPH::Shape> CreateLedgeShape(const LedgePoint& instance, const LedgePoint& next)
        {
            const glm::vec3 line = next.point - instance.point;
            const JPH::Vec3 extents = GlmVec3ToJph({ 0.5f * glm::length(line), 1.0f, 1.0f });
            const JPH::Vec3 position = GlmVec3ToJph(instance.point + 0.5f * line);
            const JPH::Quat rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), std::atan2(line.x, line.z));
            return new JPH::RotatedTranslatedShape(position, rotation, new JPH::BoxShape(extents));
        }

        void InitializeColliders(EngineContext& ctx)
        {
            for (auto& box : ctx.level->boxColliders)
            {
                if (box.halfExtents.x <= 0.0f || box.halfExtents.y <= 0.0f || box.halfExtents.z <= 0.0f)
                {
                    LOG_WARNING("Could not form box collider");
                    continue;
                }

                if (box.halfExtents.x <= JPH::cDefaultConvexRadius)
                {
                    box.halfExtents.x = JPH::cDefaultConvexRadius + 0.001f;
                }

                if (box.halfExtents.y <= JPH::cDefaultConvexRadius)
                {
                    box.halfExtents.y = JPH::cDefaultConvexRadius + 0.001f;
                }

                if (box.halfExtents.z <= JPH::cDefaultConvexRadius)
                {
                    box.halfExtents.z = JPH::cDefaultConvexRadius + 0.001f;
                }

                JPH::Ref<JPH::Shape> shape = new JPH::BoxShape(GlmVec3ToJph(box.halfExtents));
                JPH::Ref<JPH::Shape> shapeMoved = new JPH::RotatedTranslatedShape(GlmVec3ToJph(box.transform.position), GlmQuatToJph(box.transform.rotation), shape);
                auto& bodies = ctx.physics.system->GetBodyInterface();
                box.rigidbody = CreateBody(bodies, shapeMoved, JPH::EMotionType::Static, 0);
            }

            for (size_t m = 0; m < ctx.level->meshColliders.size(); m++)
            {
                auto& instance = ctx.level->meshColliders[m];
                auto& mesh = *ctx.level->collisionMeshes[instance.mesh];

                if (mesh.indices.size() * 3 != mesh.vertices.size())
                {
                    continue;
                }

                JPH::MeshShapeSettings settings{};
                settings.mTriangleVertices = mesh.vertices;
                settings.mIndexedTriangles = mesh.indices;
                settings.Sanitize();

                JPH::Shape::ShapeResult result = settings.Create();
                if (result.IsValid())
                {
                    ColliderHandle d{};
                    d.bodyType = COLLIDER_MESH;
                    d.index = static_cast<uint32_t>(m);
                    auto& bodies = ctx.physics.system->GetBodyInterface();
                    instance.rigidbody = CreateBody(bodies, result.Get(), JPH::EMotionType::Static, d.data);
                }
                else
                {
                    LOG_ERROR(result.GetError().c_str());
                }
            }

            for (size_t l = 0; l < ctx.level->ledges.size(); l++)
            {
                auto& instance = ctx.level->ledges[l];
                if (instance.nextLedge == 0)
                {
                    // Reached the end of the linked list
                    continue;
                }
                const auto& next = ctx.level->ledges[instance.nextLedge];

                const glm::vec3 line = next.point - instance.point;
                const JPH::Vec3 extents = GlmVec3ToJph({ 0.5f * glm::length(line), 1.0f, 1.0f });
                const JPH::Vec3 position = GlmVec3ToJph(instance.point + 0.5f * line);
                const JPH::Quat rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), std::atan2(line.x, line.z));
                const JPH::Ref<JPH::Shape> ledgeBox = new JPH::RotatedTranslatedShape(position, rotation, new JPH::BoxShape(extents));

                ColliderHandle d{};
                d.bodyType = COLLIDER_LEDGE;
                d.index = static_cast<uint32_t>(l);

                auto& bodies = ctx.physics.system->GetBodyInterface();
                instance.bodyId = CreateBody(bodies, ledgeBox, JPH::EMotionType::Static, d.data, true);
            }
        }

        void RotateCameraToMouse(EngineContext& ctx)
        {
            float deltaMouseX = ctx.mouseX - ctx.lastMouseX;
            float deltaMouseY = ctx.mouseY - ctx.lastMouseY;

            ctx.cameraPitch -= deltaMouseY * ctx.deltaTime;
            ctx.cameraYaw -= deltaMouseX * ctx.deltaTime;

            ctx.cameraPitch = Maths::Clamp(ctx.cameraPitch, glm::radians(-89.0f), glm::radians(89.0f));

            const glm::quat cameraRotation({ ctx.cameraPitch, ctx.cameraYaw, 0.0f });
            ctx.camera.transform.rotation = glm::slerp(ctx.camera.transform.rotation, cameraRotation, ctx.deltaTime * 24.0f);
        }

        void SmoothDamp(
            glm::vec3& current, glm::vec3& velocity, const glm::vec3& target,
            float smoothTime, float deltaTime)
        {
            float omega = 2.0f / smoothTime;
            float x = omega * deltaTime;
            float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

            glm::vec3 change = current - target;
            glm::vec3 temp = (velocity + omega * change) * deltaTime;
            velocity = (velocity - omega * temp) * exp;
            current = target + (change + temp) * exp;
        }

        void UpdateCamera(EngineContext& ctx)
        {
            // With free camera, we want to move only when the right mouse button is held
            if (!ctx.isFreeCamera || glfwGetMouseButton(ctx.window, 1) == GLFW_PRESS)
            {
                RotateCameraToMouse(ctx);
            }

            if (!ctx.isFreeCamera)
            {
                float distance = CameraDistance;

                HitResult result{};
                Ray ray{ ctx.cameraAnchor + glm::vec3(0.0f, CameraHeight, 0.0f), -ctx.camera.transform.ForwardVector() * distance };
                if (ctx.physicsInterface.Raycast(ray, result))
                {
                    distance = glm::length(result.point - ray.origin) - 0.1f;
                }

                const float smoothTime = distance < CameraDistance ? CameraApproachRate : CameraRetreatRate;
                
                static glm::vec3 cameraVelocity{};
                SmoothDamp(ctx.cameraAnchor, cameraVelocity, ctx.lara.transform.position, smoothTime, ctx.deltaTime);

                glm::vec3 cameraTarget = ctx.cameraAnchor - ctx.camera.transform.ForwardVector() * distance + glm::vec3(0.0f, CameraHeight, 0.0f);
                ctx.camera.transform.position = cameraTarget;
            }
            else
            {
                float forwardKey = glfwGetKey(ctx.window, GLFW_KEY_W) == GLFW_PRESS ? -1.0f : 0.0f;
                float backKey = glfwGetKey(ctx.window, GLFW_KEY_S) == GLFW_PRESS ? 1.0f : 0.0f;
                float leftKey = glfwGetKey(ctx.window, GLFW_KEY_A) == GLFW_PRESS ? -1.0f : 0.0f;
                float rightKey = glfwGetKey(ctx.window, GLFW_KEY_D) == GLFW_PRESS ? 1.0f : 0.0f;
                float upKey = glfwGetKey(ctx.window, GLFW_KEY_Q) == GLFW_PRESS ? 1.0f : 0.0f;
                float downKey = glfwGetKey(ctx.window, GLFW_KEY_E) == GLFW_PRESS ? -1.0f : 0.0f;

                glm::vec3 moveVector = ctx.camera.transform.rotation * glm::vec3{ leftKey + rightKey, upKey + downKey, forwardKey + backKey };
                moveVector *= ctx.freeCameraSpeed;

                ctx.camera.transform.position += moveVector * ctx.deltaTime;
            }
        }
    }

    // ---------------------------------------------------------------
    // Engine functions
    // ---------------------------------------------------------------

    bool InitEngine(EngineContext& ctx)
    {
        DEBUG_INIT();

        const Config& config = Config::Get();

        if (!glfwInit())
        {
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        ctx.windowWidth = config.resolutionX;
        ctx.windowHeight = config.resolutionY;
        if (ctx.window = glfwCreateWindow(ctx.windowWidth, ctx.windowHeight, config.windowTitle.c_str(), nullptr, nullptr); !ctx.window)
        {
            glfwTerminate();
            return false;
        }

        glfwMaximizeWindow(ctx.window);
        glfwGetWindowSize(ctx.window, &ctx.windowWidth, &ctx.windowHeight);
        glfwMakeContextCurrent(ctx.window);
        glfwSwapInterval(0);

        double cursorX{};
        double cursorY{};
        glfwGetCursorPos(ctx.window, &cursorX, &cursorY);

        ctx.mouseX = static_cast<float>(cursorX);
        ctx.mouseY = static_cast<float>(cursorY);
        ctx.lastMouseX = ctx.mouseX;
        ctx.lastMouseY = ctx.mouseY;

        Input::SetWindow(ctx.window);
        glfwSetWindowUserPointer(ctx.window, &ctx);
        glfwSetWindowSizeCallback(ctx.window, HandleFramebufferResize);
        glfwSetCursorPosCallback(ctx.window, HandleMouseMove);
        glfwSetScrollCallback(ctx.window, HandleMouseScroll);
        glfwSetKeyCallback(ctx.window, HandleKey);
        glfwSetMouseButtonCallback(ctx.window, HandleMouseButton);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            glfwDestroyWindow(ctx.window);
            glfwTerminate();
            return false;
        }

        InitPhysics(ctx.physics);
        ctx.physicsInterface.SetSystem(ctx.physics.system);
        ctx.physicsInterface.SetObjBroadPhaseFilter(&ctx.physics.objVsBpLayerFilter);
        ctx.physicsInterface.SetObjectLayerPairFilter(&ctx.physics.objVsObjLayerFilter);
        ctx.physicsInterface.SetPlayerBpFilter(&ctx.physics.playerBpFilter);
        ctx.physicsInterface.SetPlayerLayerFilter(&ctx.physics.playerLayerFilter);

        ctx.renderer.Initialize(ctx.windowWidth, ctx.windowHeight);
        ctx.camera.aspect = static_cast<float>(ctx.windowWidth) / ctx.windowHeight;
        SetupDefaultShapes(ctx);

        ctx.previousTime = glfwGetTime();

        return true;
    }

    bool UpdateEngine(EngineContext& ctx)
    {
        if (glfwWindowShouldClose(ctx.window) || ctx.shouldQuit)
        {
            return false;
        }

        const double currentTime = glfwGetTime();
        ctx.deltaTime = Maths::Clamp(static_cast<float>(currentTime - ctx.previousTime), 0.0f, MaxDeltaTime);

        ctx.renderer.ClearFramebuffer();

        if (ctx.level)
        {
            UpdateCamera(ctx); // Free camera can still update if paused
            ctx.audioSystem.SetListenerPosition(ctx.camera.transform.position.x, ctx.camera.transform.position.y, ctx.camera.transform.position.z);
            ctx.audioSystem.SetListenerDirection(
                ctx.camera.transform.ForwardVector().x,
                ctx.camera.transform.ForwardVector().y,
                ctx.camera.transform.ForwardVector().z,
                ctx.camera.transform.UpVector().x,
                ctx.camera.transform.UpVector().y,
                ctx.camera.transform.UpVector().z);

            if (!ctx.isPaused || ctx.wantsFrameAdvance)
            {
                ctx.totalTime += ctx.deltaTime;
                ctx.wantsFrameAdvance = false;
                ctx.lara.cameraPitch = ctx.cameraPitch;
                ctx.lara.cameraYaw = ctx.cameraYaw;
                ctx.laraController.Update(ctx.deltaTime, ctx.physics);
                ctx.physics.system->Update(ctx.deltaTime, 1, ctx.physics.tmpAllocator, ctx.physics.jobSystem);
            }

            ctx.renderer.RenderLevel(*ctx.level, ctx.lara, ctx.camera);
        }

        ctx.audioSystem.Update(ctx.deltaTime);

#if EDITOR_ENABLED
        if (currentTime - ctx.debugData.lastFpsUpdate > 1.0)
        {
            ctx.debugData.fps = ctx.debugData.framesThisSecond;
            ctx.debugData.lastFpsUpdate = currentTime;
            ctx.debugData.framesThisSecond = 0;
        }
        ctx.debugData.framesThisSecond++;
#endif

        ctx.lastMouseX = ctx.mouseX;
        ctx.lastMouseY = ctx.mouseY;
        ctx.previousTime = currentTime;

        return true;
    }

    void SwapBuffers(EngineContext& ctx)
    {
        glfwSwapBuffers(ctx.window);
    }

    void PollEvents(EngineContext& ctx)
    {
        glfwPollEvents();
    }

    void DestroyEngine(EngineContext& ctx)
    {
        UnloadLevel(ctx);
        ctx.renderer.Destroy();

        DestroyPhysics(ctx.physics);

        glfwDestroyWindow(ctx.window);
        glfwTerminate();
    }

    void LoadLevel(EngineContext& ctx, const AssetId path)
    {
        UnloadLevel(ctx);

        ctx.level = ctx.assetRegistry.Load<Level>(path);

        if (ctx.level)
        {
            InitializeColliders(ctx);
            ctx.renderer.InitializeLevel(*ctx.level);
            ctx.lara.transform.position = ctx.level->startPosition;
            ctx.laraController.SetLevel(ctx.level.get());
        }
        else
        {
            LOG_ERROR("Failed to load level: %i", path);
        }
    }

    void UnloadLevel(EngineContext& ctx)
    {
        if (ctx.level)
        {
            // Clear physics bodies
            for (auto& obj : ctx.level->boxColliders)
            {
                auto bodyId = obj.rigidbody;
                if (!bodyId.IsInvalid())
                {
                    auto& bodies = ctx.physics.system->GetBodyInterface();
                    bodies.RemoveBody(bodyId);
                    bodies.DestroyBody(bodyId);
                    obj.rigidbody = JPH::BodyID();
                }
            }
            for (auto& obj : ctx.level->meshColliders)
            {
                auto bodyId = obj.rigidbody;
                if (!bodyId.IsInvalid())
                {
                    auto& bodies = ctx.physics.system->GetBodyInterface();
                    bodies.RemoveBody(bodyId);
                    bodies.DestroyBody(bodyId);
                    obj.rigidbody = JPH::BodyID();
                }
            }
            ctx.level->boxColliders.clear();
            ctx.level->meshColliders.clear();

            ctx.renderer.DeloadLevel(*ctx.level);
            ctx.level->meshes.clear();
        }

        ctx.level = nullptr;
    }

    void DeleteLevelObject(EngineContext& ctx, size_t index)
    {
        uint32_t colliderToRemove = ctx.level->meshes[index].collision.index;
        ColliderType colliderType = ctx.level->meshes[index].collision.bodyType;

        JPH::BodyID bodyId{};
        if (colliderType == COLLIDER_MESH)
        {
            bodyId = ctx.level->meshColliders[colliderToRemove].rigidbody;
            ctx.level->meshColliders[colliderToRemove].rigidbody = JPH::BodyID();
        }
        else if (colliderType == COLLIDER_BOX)
        {
            bodyId = ctx.level->boxColliders[colliderToRemove].rigidbody;
            ctx.level->boxColliders[colliderToRemove].rigidbody = JPH::BodyID();
        }

        if (!bodyId.IsInvalid())
        {
            auto& bodies = ctx.physics.system->GetBodyInterface();
            bodies.RemoveBody(bodyId);
            bodies.DestroyBody(bodyId);
        }

        ctx.level->meshes[index] = {};
    }

    void SetLaraModel(EngineContext& ctx, const AssetId modelId)
    {
        SetupLara(ctx, modelId);
    }

    void SetMouseVisible(GLFWwindow* window, bool visible)
    {
        if (visible)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    void UpdateLedge(EngineContext& ctx, size_t index, size_t nextLedge, const glm::vec3& point)
    {
        auto& ledge = ctx.level->ledges[index];
        ledge.nextLedge = static_cast<uint32_t>(nextLedge);
        ledge.point = point;

        if (nextLedge > 0 && nextLedge != index && nextLedge < ctx.level->ledges.size())
        {
            JPH::Ref<JPH::Shape> shape = CreateLedgeShape(ctx.level->ledges[index], ctx.level->ledges[nextLedge]);

            auto& bodies = ctx.physics.system->GetBodyInterface();
            if (ledge.bodyId.IsInvalid())
            {
                ColliderHandle d{};
                d.bodyType = COLLIDER_LEDGE;
                d.index = static_cast<uint32_t>(index);
                ledge.bodyId = CreateBody(bodies, shape, JPH::EMotionType::Static, d.data, true);
            }
            else
            {
                bodies.SetShape(
                    ctx.level->ledges[index].bodyId, 
                    CreateLedgeShape(ctx.level->ledges[index], ctx.level->ledges[nextLedge]),
                    false,
                    JPH::EActivation::Activate);
            }
        }
    }
}
