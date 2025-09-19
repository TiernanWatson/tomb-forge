#include "Engine/Engine.h"

#include <cmath>

#include <glad/glad.h>
#include <glfw3.h>
#include <glm/glm.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
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
                ctx->renderer->OnWindowResized(width, height);
            }
        }

        void GenerateDebugTexture(Texture& texture, glm::ivec4 background, glm::ivec4 line)
        {
            constexpr int LineThickness = 10;

            texture.width = texture.height = 1024;
            texture.format = TextureFormat::RGBA;

            texture.data.resize(texture.width * texture.height * 4);

            for (int w = 0; w < texture.width; w++)
            {
                for (int h = 0; h < texture.height; h++)
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
            debugMaterial->diffuse = debugTexture;
            debugMaterial->AddFlag(MATERIAL_FLAG_DIFFUSE);
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
            ctx.renderer->InitializeModel(*cube);

            // Cone

            std::shared_ptr<Model> cone = std::make_shared<Model>();
            MakeUnitCone(*cone, 4);
            cone->meshes[0].material = debugMaterial;
            cone->id = ConeId;
            cone->name = "Cone";
            ctx.assetRegistry.AddAssetBuiltin(cone);
            ctx.renderer->InitializeModel(*cone);

            // Arrow

            std::shared_ptr<Model> arrow = std::make_shared<Model>();
            MakeUnitArrow(*arrow);
            arrow->meshes[0].material = debugMaterial;
            arrow->id = ArrowId;
            arrow->name = "Arrow";
            ctx.assetRegistry.AddAssetBuiltin(arrow);
            ctx.renderer->InitializeModel(*arrow);
        }

        JPH::BodyID CreateBody(JPH::BodyInterface& bodies, JPH::Ref<JPH::Shape> shape, JPH::EMotionType motion, JPH::uint64 userData)
        {
            JPH::BodyCreationSettings settings{};
            settings.mMotionType = motion;
            settings.SetShape(shape);
            settings.mUserData = userData;

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

            ctx.renderer->InitializeModel(*ctx.lara.model);

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

            ctx.lara.physics = new JPH::CharacterVirtual(characterSettings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, ctx.physics.system);

            ctx.lara.states.reserve(LARA_STATE_COUNT);
            for (size_t i = 0; i < LARA_STATE_COUNT; i++)
            {
                LaraBaseState* state{};
                switch (i)
                {
                case LARA_STATE_LOCOMOTION:
                    state = new LocomotionState();
                    break;
                case LARA_STATE_AIR:
                    state = new AirState();
                    break;
                case LARA_STATE_CLIMB:
                    state = new ClimbState();
                    break;
                default:
                    LOG_ERROR("Could not set up state %i", i);
                    break;
                }
                ctx.lara.states.emplace(ctx.lara.states.begin() + i, state);
            }

            ctx.lara.LoadAnimations(ctx.assetRegistry);
            ctx.lara.SetAnimation(LARA_ANIM_IDLE, 0.0f, true);
        }

        void InitializeColliders(EngineContext& ctx)
        {
            /*for (auto& box : m_level->boxColliders)
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
                auto& bodies = m_physicsSystem->GetBodyInterface();
                box.rigidbody = CreateBody(bodies, shapeMoved, JPH::EMotionType::Static);
            }*/

            for (uint64_t m = 0; m < ctx.level->meshColliders.size(); m++)
            {
                auto& mesh = ctx.level->meshColliders[m];

                JPH::IndexedTriangleList triList{};
                for (size_t i = 0; i < mesh.indices.size(); i += 3)
                {
                    JPH::IndexedTriangle tri{};
                    tri.mIdx[0] = mesh.indices[i];
                    tri.mIdx[1] = mesh.indices[i + 1];
                    tri.mIdx[2] = mesh.indices[i + 2];
                    triList.emplace_back(tri);
                }

                JPH::VertexList vertList{};
                for (size_t i = 0; i < mesh.vertices.size(); i++)
                {
                    vertList.emplace_back(mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
                }

                if (triList.size() * 3 != vertList.size())
                {
                    continue;
                }

                JPH::MeshShapeSettings settings{};
                settings.mTriangleVertices = vertList;
                settings.mIndexedTriangles = triList;
                settings.Sanitize();

                JPH::Shape::ShapeResult result = settings.Create();

                if (result.IsValid())
                {
                    JPH::Ref<JPH::Shape> meshShape = settings.Create().Get();
                    auto& bodies = ctx.physics.system->GetBodyInterface();
                    mesh.rigidbody = CreateBody(bodies, meshShape, JPH::EMotionType::Static, m);
                }
                else
                {
                    LOG_ERROR(result.GetError().c_str());
                }
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
            ctx.camera.transform.rotation = glm::slerp(ctx.camera.transform.rotation, cameraRotation, ctx.deltaTime * 30.0f);
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
                glm::vec3 cameraTarget = ctx.lara.transform.position
                    - ctx.camera.transform.ForwardVector() * 4.0f
                    + glm::vec3(0.0f, 1.25f, 0.0f);

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
            throw std::runtime_error("Failed to initialize GLFW");
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
            throw std::runtime_error("Failed to initialize the window");
        }

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
            throw std::runtime_error("Failed to initialize GLAD");
        }

        InitPhysics(ctx.physics);

        ctx.renderer = std::make_unique<Renderer>();
        ctx.camera.aspect = static_cast<float>(ctx.windowWidth) / ctx.windowHeight;
        SetupDefaultShapes(ctx);

        ctx.previousTime = glfwGetTime();
    }

    bool UpdateEngine(EngineContext& ctx)
    {
        if (glfwWindowShouldClose(ctx.window) || ctx.shouldQuit)
        {
            return false;
        }

        const double currentTime = glfwGetTime();
        ctx.deltaTime = Maths::Clamp(static_cast<float>(currentTime - ctx.previousTime), 0.0f, MaxDeltaTime);

        ctx.renderer->ClearFramebuffer();

        if (ctx.level)
        {
            UpdateCamera(ctx); // Free camera can still update if paused

            if (!ctx.isPaused || ctx.wantsFrameAdvance)
            {
                ctx.totalTime += ctx.deltaTime;
                ctx.wantsFrameAdvance = false;

                if (ctx.lara.model)
                {
                    ctx.lara.cameraPitch = ctx.cameraPitch;
                    ctx.lara.cameraYaw = ctx.cameraYaw;

                    auto* state = ctx.lara.states[ctx.lara.stateIndex].get();
                    if (ctx.lara.stateIndex != LARA_STATE_COUNT)
                    {
                        if (LaraState nextState = state->ShouldTransition(ctx.laraController); nextState != LARA_STATE_COUNT)
                        {
                            if (nextState < ctx.lara.states.size() && ctx.lara.states[nextState])
                            {
                                state->Exit(ctx.laraController);
                                state = ctx.lara.states[nextState].get();
                                state->Begin(ctx.laraController);

                                ctx.lara.stateIndex = nextState;
                            }
                        }

                        state->PreAnimationUpdate(ctx.laraController, ctx.deltaTime);
                        state->UpdateAnimation(ctx.laraController, ctx.deltaTime);
                        ctx.lara.animPlayer.Process(ctx.deltaTime);
                        state->PostAnimationUpdate(ctx.laraController, ctx.deltaTime);
                    }
                }

                auto& character = ctx.lara.physics;
                if (character)
                {
                    auto& state = ctx.lara.states[ctx.lara.stateIndex];
                    state->PrePhysicsUpdate(ctx.laraController, ctx.deltaTime, ctx.physicsInterface);

                    const JPH::Vec3 velocity = character->GetGroundVelocity() + GlmVec3ToJph(ctx.lara.actualVelocity);
                    character->SetPosition(GlmVec3ToJph(ctx.lara.transform.position));
                    character->SetLinearVelocity(velocity);
                    character->ExtendedUpdate(ctx.deltaTime,
                        { 0.0f, -9.8f, 0.0f },
                        {},
                        ctx.physics.playerBpFilter,
                        ctx.physics.playerLayerFilter,
                        { },
                        { },
                        *ctx.physics.tmpAllocator);

                    ctx.lara.transform.position = JphVec3ToGlm(character->GetPosition());
                    state->PostPhysicsUpdate(ctx.laraController, ctx.deltaTime, ctx.physicsInterface);
                }

                ctx.physics.system->Update(ctx.deltaTime, 1, ctx.physics.tmpAllocator, ctx.physics.jobSystem);
            }

            ctx.renderer->RenderLevel(*ctx.level, ctx.lara, ctx.camera);
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
            ctx.renderer->InitializeLevel(*ctx.level);
            InitializeColliders(ctx);
            ctx.lara.transform.position = ctx.level->startPosition;
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

            ctx.renderer->DeloadLevel(*ctx.level);
            ctx.level->meshes.clear();
        }

        ctx.level = nullptr;
    }

    void DeleteLevelObject(EngineContext& ctx, size_t index)
    {
        ColliderId colliderToRemove = ctx.level->meshes[index].collision.id;
        ColliderType colliderType = ctx.level->meshes[index].collision.type;
        if (colliderType == COLLIDER_MESH)
        {
            auto bodyId = ctx.level->meshColliders[colliderToRemove].rigidbody;
            if (!bodyId.IsInvalid())
            {
                auto& bodies = ctx.physics.system->GetBodyInterface();
                bodies.RemoveBody(bodyId);
                bodies.DestroyBody(bodyId);
                ctx.level->meshColliders[colliderToRemove].rigidbody = JPH::BodyID();
            }
        }
        else if (colliderType == COLLIDER_BOX)
        {
            auto bodyId = ctx.level->boxColliders[colliderToRemove].rigidbody;
            if (!bodyId.IsInvalid())
            {
                auto& bodies = ctx.physics.system->GetBodyInterface();
                bodies.RemoveBody(bodyId);
                bodies.DestroyBody(bodyId);
                ctx.level->boxColliders[colliderToRemove].rigidbody = JPH::BodyID();
            }
        }

        ctx.level->meshes[index] = {};
    }

    void SetLaraModel(EngineContext& ctx, const AssetId modelId)
    {
        SetupLara(ctx, modelId);
    }

    void SetAndInitColliders(EngineContext& ctx, std::vector<BoxCollider>&& colliders)
    {
        ctx.level->boxColliders = std::move(colliders);
        InitializeColliders(ctx);
    }

    void SetAndInitColliders(EngineContext& ctx, std::vector<MeshCollider>&& colliders)
    {
        ctx.level->meshColliders = std::move(colliders);
        InitializeColliders(ctx);
    }
}
