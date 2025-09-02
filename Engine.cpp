#include "Engine.h"

#include <stdexcept>

#include <fstream>
#include <glad/glad.h>
#include <glfw3.h>
#include <string>

#include "Core/Config.h"
#include "Core/Debug.h"
#include "Core/IO/DevIO.h"
#include "Core/IO/FileIO.h"
#include "Core/Maths/Maths.h"
#include "Core/Graphics/Color.h"
#include "Engine/Assets/GmxImport.h"
#include "Engine/Assets/ModelImporter.h"
#include "Engine/Assets/TextureImport.h"
#include "Engine/Levels/Level.h"
#include "Engine/Player/Input.h"
#include "Engine/Player/Lara.h"
#include "Engine/Player/States/AirState.h"
#include "Engine/Player/States/ClimbState.h"
#include "Engine/Player/States/LocomotionState.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/Texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>

#if DEVSLATE

#include "Core/IO/DevIO.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// These Win32 macros conflict with camera code
#undef near
#undef far

#endif

namespace TombForge
{
    static constexpr float MouseSpeedIncrease = 0.25f;
    static constexpr float CameraPitchMax = glm::radians(85.0f);
    static constexpr float CameraPitchMin = glm::radians(-85.0f);

    static constexpr char const* SkeletonFileExt{ ".tfskel" };
    static constexpr char const* AnimFileExt{ ".tfanim" };
    static constexpr char const* ModelFileExt{ ".tfmod" };
    static constexpr char const* TextureFileExt{ ".tftex" };
    static constexpr char const* MaterialFileExt{ ".tfmat" };
    static constexpr char const* LevelFileExt{ ".tflev" };
    static constexpr char const* ProjectFileExt{ ".tfproj" };

    static constexpr unsigned int NumTombSlateFiles{ 1 };
    static constexpr unsigned int NumImportFiles{ 1 };
    static constexpr unsigned int NumAodFiles{ 1 };
    static constexpr unsigned int NumTextureFiles{ 1 };

    static constexpr COMDLG_FILTERSPEC TombSlateFileTypes[] =
    {
        { L"TombForge Asset", L"*.tombs;*.tfskel;*.tfanim;*.tfmod;*.tftex;*.tfmat;*.tflev;*.tfproj" }
    };

    static constexpr COMDLG_FILTERSPEC ModelFileTypes[] =
    {
        { L"Supported Model Types", L"*.fbx;*.obj;*.3ds;*.dae;*.gltf;*.glb" }
    };

    static constexpr COMDLG_FILTERSPEC AodFileTypes[] =
    {
        { L"AOD Level File", L"*.gmx;*.gmx.clz" }
    };

    static constexpr COMDLG_FILTERSPEC TextureFileTypes[] =
    {
        { L"Supported Texture Types", L"*.jpg;*.jpeg;*.tga;*.png;*.bmp;*.tif" }
    };

    // Callbacks

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        if (Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window)); engine)
        {
            engine->HandleFramebufferResize(width, height);
        }
    }

    static void MouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition)
    {
        float mouseX = static_cast<float>(xPosition);
        float mouseY = static_cast<float>(yPosition);

        if (Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window)); engine)
        {
            engine->HandleMouseMove(mouseX, mouseY);
        }
    }

    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
    {
        float scrollY = static_cast<float>(yOffset);

        if (Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window)); engine)
        {
            engine->HandleMouseScroll(scrollY);
        }
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        if (Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window)); engine)
        {
            engine->HandleMouseButton(button);
        }
    }

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
#if DEVSLATE
        if (Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window)); engine)
        {
            // F3 - Enter editor or play mode
            // F4 - Draw Colliders
            // F5 - Draw Wireframe
            // F6 - Draw Mesh Normals
            // F7 - Draw Rendering Octree
            // F8 - Advance one frame
            // Escape - Close

            if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
            {
                engine->SetDevMode(!engine->IsDevMode());
            }
            else if (key == GLFW_KEY_F4 && action == GLFW_PRESS)
            {
                engine->ShowColliders(!engine->IsShowColliders());
            }
            else if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
            {
                engine->ShowWireframe(!engine->IsShowWireframe());
            }
            else if (key == GLFW_KEY_F6 && action == GLFW_PRESS)
            {
                engine->DrawNormals(!engine->IsDrawNormals());
            }
            else if (key == GLFW_KEY_F7 && action == GLFW_PRESS)
            {
                engine->DrawOctree(!engine->IsDrawOctree());
            }
            else if (key == GLFW_KEY_F8 && action == GLFW_PRESS)
            {
                engine->RequestFrame(engine->IsDevMode());
            }
            else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                engine->RequestShutdown();
            }
        }
#endif
    }

    void GenerateDebugTexture(Texture& texture, glm::ivec4 background, glm::ivec4 line)
    {
        constexpr int LineThickness = 10;

        texture.width = texture.height = 1024;
        texture.format = TextureFormat::RGBA;

        texture.data.resize(texture.width * texture.height * 4);

        for (int w = 0; w < 1024; w++)
        {
            for (int h = 0; h < 1024; h++)
            {
                const size_t baseIndex = (w * 1024 * 4) + h * 4;

                if (w < LineThickness || h < LineThickness || w >(1024 - LineThickness) || h >(1024 - LineThickness))
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

    // Engine functions

    Engine::Engine()
        : m_laraController{ &m_lara, &m_physicsInterface }
    {
        const Config& config = Config::Get();

        m_windowWidth = config.resolutionX;
        m_windowHeight = config.resolutionY;

        // Initialize Window

        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        if (m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, config.windowTitle.c_str(), nullptr, nullptr); !m_window)
        {
            throw std::runtime_error("Failed to initialize the window");
        }

        glfwMakeContextCurrent(m_window);

        double cursorX{};
        double cursorY{};
        glfwGetCursorPos(m_window, &cursorX, &cursorY);

        m_mouseX = static_cast<float>(cursorX);
        m_mouseY = static_cast<float>(cursorY);

        glfwSetWindowUserPointer(m_window, this); // Retrieved by the callbacks below
        glfwSetWindowSizeCallback(m_window, FramebufferSizeCallback);
        glfwSetCursorPosCallback(m_window, MouseMoveCallback);
        glfwSetScrollCallback(m_window, ScrollCallback);
        glfwSetKeyCallback(m_window, KeyCallback);
        glfwSetMouseButtonCallback(m_window, MouseButtonCallback);

        m_previousTime = glfwGetTime();

        // Input

        Input::SetWindow(m_window);

        // Initialize Physics

        JPH::RegisterDefaultAllocator();

        m_physicsSystem = new JPH::PhysicsSystem();
        m_physicsDebugRenderer = new JoltDebugRenderer();

        JPH::Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

        JPH::Factory::sInstance = new JPH::Factory();

        JPH::RegisterTypes();

        m_physicsTmpAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

        // Note: replace this with own implementation if ever implementing a job system
        m_physicsJobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        m_physicsSystem->Init(config.maxPhysicsBodies, 
            config.numPhysicsBodyMutexes, 
            config.maxPhysicsBodyPairs, 
            config.maxPhysicsContactConstraints, 
            m_bpLayerInterface, 
            m_objVsBpLayerFilter, 
            m_objVsObjLayerFilter);

        m_physicsInterface.SetSystem(m_physicsSystem);
        m_physicsInterface.SetObjBroadPhaseFilter(&m_objVsBpLayerFilter);
        m_physicsInterface.SetObjectLayerPairFilter(&m_objVsObjLayerFilter);
        m_physicsInterface.SetPlayerBpFilter(&m_playerBpFilter);
        m_physicsInterface.SetPlayerLayerFilter(&m_playerLayerFilter);

#if DEVSLATE
        m_physicsInterface.SetDebugRenderer(m_physicsDebugRenderer);
#endif

        // Renderer

        m_renderer = std::make_unique<Renderer>();

        // Levels

        SetupDefaultShapes();

        if (!m_level)
        {
            m_level = std::make_shared<Level>();
            m_level->name = "Untitled Level";
        }

        InitializeCameraRotations();

        m_camera.aspect = static_cast<float>(m_windowWidth) / m_windowHeight;

#if DEVSLATE
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init();
#endif
    }

    Engine::~Engine()
    {
#if DEVSLATE
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif

        glfwTerminate();
    }

    bool Engine::Update()
    {
        if (!glfwWindowShouldClose(m_window) && !m_shouldQuit)
        {
            const double currentTime = glfwGetTime();
            const float deltaTime = static_cast<float>(currentTime - m_previousTime);

            if (m_level)
            {
                ProcessLevelFrame(*m_level, deltaTime);

                m_physicsSystem->Update(deltaTime, 1, m_physicsTmpAllocator, m_physicsJobSystem);

                m_renderer->Render(*m_level, m_lara, m_camera);

#if DEVSLATE
                if (m_showColliders)
                {
                    if (m_lara.physics)
                    {
                        m_lara.physics->GetShape()->Draw(
                            m_physicsDebugRenderer,
                            m_lara.physics->GetCenterOfMassTransform(),
                            JPH::Vec3::sReplicate(1.0f),
                            JPH::Color::sWhite,
                            false, true);
                    }

                    for (auto& obj : m_level->boxColliders)
                    {
                        JPH::BodyInterface& bodies = m_physicsSystem->GetBodyInterface();
                        JPH::ShapeRefC shape = bodies.GetShape(obj.rigidbody);
                        if (shape)
                        {
                            shape->Draw(m_physicsDebugRenderer, bodies.GetCenterOfMassTransform(obj.rigidbody), JPH::Vec3{ 1.0f, 1.0f, 1.0f }, JPH::Color{ 0,255,0,255 }, false, true);
                        }
                    }

                    for (auto& obj : m_level->meshColliders)
                    {
                        JPH::BodyInterface& bodies = m_physicsSystem->GetBodyInterface();
                        JPH::ShapeRefC shape = bodies.GetShape(obj.rigidbody);
                        if (shape)
                        {
                            shape->Draw(m_physicsDebugRenderer, bodies.GetCenterOfMassTransform(obj.rigidbody), JPH::Vec3{ 1.0f, 1.0f, 1.0f }, JPH::Color{ 0,255,0,255 }, false, true);
                        }
                    }

                    m_physicsDebugRenderer->SubmitLines(m_camera.transform, m_camera.fovY, m_camera.aspect, m_camera.near, m_camera.far);
                    m_physicsDebugRenderer->ClearLines();
                }

                if (m_showMeshWireframe)
                {
                    if (m_level->meshes.size() > 0 && m_editInfo.selectedObject < m_level->meshes.size())
                    {
                        auto& obj = m_level->meshes[m_editInfo.selectedObject];
                        m_renderer->RenderWireframe(*obj.mesh, obj.transform, m_camera);
                    }
                }

                if (m_drawMeshNormals)
                {
                    /*if (m_level->staticObjects.size() > 0 && m_editInfo.selectedObject < m_level->staticObjects.size())
                    {
                        auto& obj = m_level->staticObjects[m_editInfo.selectedObject];
                        for (auto& mesh : obj.model->meshes)
                        {
                            for (auto& v : mesh.vertices)
                            {
                                glm::vec3 transformedPos = obj.transform.AsMatrix() * glm::vec4(v.position, 1.0f);
                                glm::vec3 end = transformedPos + v.normal;
                                m_physicsDebugRenderer->DrawColoredLine(transformedPos, end, { 0.0f, 0.0f, 1.0f, 1.0f });
                            }
                        }
                    }*/
                }

                if (m_drawOctree)
                {
                    m_renderer->DrawOctree(glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, m_camera/*, m_editInfo.selectedObject*/);

                    if (m_editInfo.selectedObject < m_level->meshes.size())
                    {
                        m_renderer->DrawBox(m_level->meshes[m_editInfo.selectedObject].bounds, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, m_camera);
                    }
                }
#endif
            }

#if DEVSLATE
            if (currentTime - m_lastFpsUpdate > 1.0)
            {
                m_fps = m_framesThisSecond;
                m_lastFpsUpdate = currentTime;
                m_framesThisSecond = 0;
            }
            m_framesThisSecond++;

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();

            ImGui::NewFrame();

            if (m_showDevWindow)
            {
                DrawGizmos();
                DrawDevWindows();
            }

            // This is done outside of if statement to stop lag happening from input
            glDisable(GL_FRAMEBUFFER_SRGB); // ImGui does not use linear space
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glEnable(GL_FRAMEBUFFER_SRGB);
#endif

            glfwSwapBuffers(m_window);
            glfwPollEvents();

            m_previousTime = currentTime;

            return true;
        }

        return false;
    }

    void Engine::HandleMouseMove(float x, float y)
    {
        m_mouseX = x;
        m_mouseY = y;
    }

    void Engine::HandleMouseScroll(float scrollY)
    {
        m_cameraSpeed += scrollY * MouseSpeedIncrease;
    }

    void Engine::HandleMouseButton(int button)
    {
        if (!m_level)
        {
            return;
        }

        if (button == GLFW_MOUSE_BUTTON_1 && !ImGui::GetIO().WantCaptureMouse)
        {
            // Convert to NDC
            float x = (2.0f * m_mouseX) / m_windowWidth - 1.0f;
            float y = 1.0f - (2.0f * m_mouseY) / m_windowHeight;
            glm::vec4 rayNDC(x, y, -1.0f, 1.0f);

            // Unproject to world space
            const glm::mat4 view = m_camera.transform.AsMatrix();
            const glm::mat4 projection = glm::perspective(m_camera.fovY, m_camera.aspect, m_camera.near, m_camera.far);

            glm::mat4 invVP = glm::inverse(projection * view);
            glm::vec4 rayWorld = invVP * rayNDC;
            rayWorld /= rayWorld.w;

            glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld) - m_camera.transform.position);

            size_t selected = 0;
            bool found = false;
            float shortestDis = std::numeric_limits<float>::max();
            for (size_t i = 0; i < m_level->meshes.size(); i++)
            {
                const auto& obj = m_level->meshes[i];

                float distance{};
                if (RayIntersectsAABB(obj.bounds, m_camera.transform.position, rayDir, &distance))
                {
                    if (distance < shortestDis)
                    {
                        shortestDis = distance;
                        selected = i;
                        found = true;
                    }
                }
            }

            if (found)
            {
                m_editInfo.selectedObject = selected;
            }
        }
    }

    void Engine::HandleFramebufferResize(int width, int height)
    {
        if (width < 1 || height < 1)
        {
            // Probably minimized
            return;
        }

        m_windowWidth = width;
        m_windowHeight = height;

        m_renderer->OnWindowResized(width, height);

        m_camera.aspect = static_cast<float>(width) / height;
    }

    void Engine::InitializeCameraRotations()
    {
        double mouseX{};
        double mouseY{};

        glfwGetCursorPos(m_window, &mouseX, &mouseY);

        m_lastMouseX = static_cast<float>(mouseX);
        m_lastMouseY = static_cast<float>(mouseY);
    }

    void Engine::RotateCameraToMouse(float deltaTime)
    {
        float deltaMouseX = m_mouseX - m_lastMouseX;
        float deltaMouseY = m_mouseY - m_lastMouseY;

        m_cameraPitch -= deltaMouseY * deltaTime;
        m_cameraYaw -= deltaMouseX * deltaTime;

        m_cameraPitch = Maths::Clamp(m_cameraPitch, CameraPitchMin, CameraPitchMax);

        const glm::quat cameraRotation({ m_cameraPitch, m_cameraYaw, 0.0f });
        m_camera.transform.rotation = glm::slerp(m_camera.transform.rotation, cameraRotation, deltaTime * 30.0f);
    }

    void Engine::ProcessLevelFrame(Level& level, float deltaTime)
    {
#if DEVSLATE
        const bool isPlayer = !m_showDevWindow;
#else
        constexpr bool isPlayer = true;
#endif

        // Rotation
        if (isPlayer || glfwGetMouseButton(m_window, 1) == GLFW_PRESS)
        {
            RotateCameraToMouse(deltaTime);
        }

        m_lastMouseX = m_mouseX;
        m_lastMouseY = m_mouseY;

        if (m_lara.model && (isPlayer || m_wantsFrameAdvance))
        {
            m_lara.cameraPitch = m_cameraPitch;
            m_lara.cameraYaw = m_cameraYaw;

            states[m_stateIndex]->PrePhysicsUpdate(m_laraController, deltaTime, m_physicsInterface);

            UpdateLara(deltaTime);
            UpdatePlayerPhysics(deltaTime);

            states[m_stateIndex]->PostPhysicsUpdate(m_laraController, deltaTime, m_physicsInterface);

            if (isPlayer)
            {
                glm::vec3 cameraTarget = m_lara.transform.position
                    - m_camera.transform.ForwardVector() * 4.0f
                    + glm::vec3(0.0f, 1.25f, 0.0f);

                m_camera.transform.position = cameraTarget;
            }

            m_wantsFrameAdvance = false;
        }
        
        // Separate as the above can fire if a frame is requested
        if (!isPlayer)
        {
            float forwardKey = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS ? -1.0f : 0.0f;
            float backKey = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS ? 1.0f : 0.0f;
            float leftKey = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS ? -1.0f : 0.0f;
            float rightKey = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS ? 1.0f : 0.0f;

            float upKey = glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS ? 1.0f : 0.0f;
            float downKey = glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS ? -1.0f : 0.0f;

            glm::vec3 moveVector = m_camera.transform.rotation * glm::vec3{ leftKey + rightKey, upKey + downKey, forwardKey + backKey };
            moveVector *= m_cameraSpeed;

            m_camera.transform.position += moveVector * deltaTime;
        }
    }

    void Engine::GoToState(Lara& lara, LaraController& controller, LaraState newState)
    {
        if (newState > states.size() || !states[newState])
        {
            LOG_ERROR("Couldn't transition to state %i", newState);
            return;
        }

        states[m_stateIndex]->Exit(controller);
        states[newState]->Begin(controller);

        m_stateIndex = newState;
    }

    void Engine::UpdateLara(float deltaTime)
    {
        if (m_stateIndex == LARA_STATE_COUNT)
        {
            return;
        }

        if (LaraState nextState = states[m_stateIndex]->ShouldTransition(m_laraController); nextState != LARA_STATE_COUNT)
        {
            GoToState(m_lara, m_laraController, nextState);
        }

        LaraBaseState* state = states[m_stateIndex].get();
        state->PreAnimationUpdate(m_laraController, deltaTime);

        state->UpdateAnimation(m_laraController, deltaTime);
        m_lara.animPlayer.Process(deltaTime);

        state->PostAnimationUpdate(m_laraController, deltaTime);
    }

    void Engine::UpdatePlayerPhysics(float deltaTime)
    {
        auto character = m_lara.physics;
        if (character)
        {
            JPH::CharacterVirtual::ExtendedUpdateSettings settings{};

            const JPH::Vec3 intendedForward = character->GetGroundVelocity() + GlmVec3ToJph(m_lara.actualVelocity);
            character->SetLinearVelocity(intendedForward);

            character->ExtendedUpdate(deltaTime,
                { 0.0f, -9.8f, 0.0f },
                settings,
                m_playerBpFilter,
                m_playerLayerFilter,
                { },
                { },
                *m_physicsTmpAllocator);

            const JPH::Vec3 resultPosition = character->GetPosition();
            m_lara.transform.position.x = resultPosition.GetX();
            m_lara.transform.position.y = resultPosition.GetY();
            m_lara.transform.position.z = resultPosition.GetZ();
        }
    }

    void Engine::HandleLaraAnimEvent(AnimEvent event)
    {
        switch (event)
        {
        case ANIM_EVENT_COLLISION_OFF:
            m_physicsInterface.SetPlayerCollidesWorld(false);
            break;
        case ANIM_EVENT_COLLISION_ON:
            m_physicsInterface.SetPlayerCollidesWorld(true);
            break;
        default:
            break;
        }
    }

    void Engine::SetupDefaultShapes()
    {
        // Texture

        std::shared_ptr<Texture> debugTexture = std::make_shared<Texture>();
        GenerateDebugTexture(*debugTexture, glm::ivec4{ 20, 50, 200, 255 }, glm::ivec4{ 20, 20, 20, 255 });
        debugTexture->name = "Debug Texture";
        m_assets.AddAssetBuiltin(debugTexture);

        // Material

        std::shared_ptr<Material> debugMaterial = std::make_shared<Material>();
        debugMaterial->diffuse = debugTexture;
        debugMaterial->AddFlag(MATERIAL_FLAG_DIFFUSE);
        debugMaterial->name = "Debug Material";
        m_assets.AddAssetBuiltin(debugMaterial);

        // Cube

        std::shared_ptr<Model> cube = std::make_shared<Model>();
        MakeUnitCube(*cube);
        cube->meshes[0].material = debugMaterial;
        cube->name = "Cube";
        m_assets.AddAssetBuiltin(cube);

        // Cone

        std::shared_ptr<Model> cone = std::make_shared<Model>();
        MakeUnitCone(*cone, 4);
        cone->meshes[0].material = debugMaterial;
        cone->name = "Cone";
        m_assets.AddAssetBuiltin(cone);

        // Arrow

        std::shared_ptr<Model> arrow = std::make_shared<Model>();
        MakeUnitArrow(*arrow);
        arrow->meshes[0].material = debugMaterial;
        arrow->name = "Arrow";
        m_assets.AddAssetBuiltin(arrow);
    }

    void Engine::SetupLara()
    {
        const AssetId laraPath = m_editInfo.projectSettings.laraPath;

        if (!IsValidAssetId(laraPath))
        {
            LOG_ERROR("Lara ID not valid: %i", laraPath);
            return;
        }

        m_lara.model = m_assets.Load<Model>(laraPath);
        if (!m_lara.model)
        {
            LOG_ERROR("Failed to load Lara model with ID: %i", laraPath);
            return;
        }

        m_lara.animPlayer.SetSkeleton(m_lara.model->skeleton);

        m_renderer->InitializeModel(*m_lara.model);

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

        m_lara.physics = new JPH::CharacterVirtual(characterSettings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, m_physicsSystem);

        states.reserve(LARA_STATE_COUNT);
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
            states.emplace(states.begin() + i, state);
        }

        m_lara.LoadAnimations(m_assets);
        m_lara.SetAnimation(LARA_ANIM_IDLE, 0.0f, true);
    }

    void Engine::LoadLevel(const AssetId path)
    {
        UnloadLevel();

        m_level = m_assets.Load<Level>(path);

        if (m_level)
        {
            m_renderer->InitializeLevel(*m_level);
            InitializeColliders();
            
            m_lara.transform.position = m_level->startPosition;
        }
        else
        {
            LOG_ERROR("Failed to load level: %i", path);
        }
    }

    void Engine::UnloadLevel()
    {
        if (m_level)
        {
            // Clear physics bodies
            for (auto& obj : m_level->boxColliders)
            {
                auto bodyId = obj.rigidbody;
                if (!bodyId.IsInvalid())
                {
                    auto& bodies = m_physicsSystem->GetBodyInterface();
                    bodies.RemoveBody(bodyId);
                    bodies.DestroyBody(bodyId);
                    obj.rigidbody = JPH::BodyID();
                }
            }
            for (auto& obj : m_level->meshColliders)
            {
                auto bodyId = obj.rigidbody;
                if (!bodyId.IsInvalid())
                {
                    auto& bodies = m_physicsSystem->GetBodyInterface();
                    bodies.RemoveBody(bodyId);
                    bodies.DestroyBody(bodyId);
                    obj.rigidbody = JPH::BodyID();
                }
            }
            m_level->boxColliders.clear();
            m_level->meshColliders.clear();

            m_renderer->DeloadLevel(*m_level);
            m_level->meshes.clear();
        }

        m_level = nullptr;
    }

    void Engine::DeleteLevelObject(size_t index)
    {
        ColliderId colliderToRemove = m_level->meshes[index].collision.id;
        ColliderType colliderType = m_level->meshes[index].collision.type;
        if (colliderType == COLLIDER_MESH)
        {
            auto bodyId = m_level->meshColliders[colliderToRemove].rigidbody;
            if (!bodyId.IsInvalid())
            {
                auto& bodies = m_physicsSystem->GetBodyInterface();
                bodies.RemoveBody(bodyId);
                bodies.DestroyBody(bodyId);
                m_level->meshColliders[colliderToRemove].rigidbody = JPH::BodyID();
            }
        }
        else if (colliderType == COLLIDER_BOX)
        {
            auto bodyId = m_level->boxColliders[colliderToRemove].rigidbody;
            if (!bodyId.IsInvalid())
            {
                auto& bodies = m_physicsSystem->GetBodyInterface();
                bodies.RemoveBody(bodyId);
                bodies.DestroyBody(bodyId);
                m_level->boxColliders[colliderToRemove].rigidbody = JPH::BodyID();
            }
        }

        m_level->meshes[index] = {};

#if DEVSLATE
        m_editInfo.selectedObject = 0;
#endif
    }

#if DEVSLATE
    void PrintDebugMessage(const Debug::DbgMessage& message)
    {
        std::string fileLineString = message.file;
        fileLineString.append(" - ");
        fileLineString.append(std::to_string(message.line));

        constexpr ImVec4 InfoColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr ImVec4 WarningColor{ 1.0f, 0.65f, 0.0f, 1.0f };
        constexpr ImVec4 ErrorColor{ 1.0f, 0.1f, 0.1f, 1.0f };

        const ImVec4 color = 
            message.verbosity == Debug::DbgVerbosity::Error ? ErrorColor
            : message.verbosity == Debug::DbgVerbosity::Warning ? WarningColor
            : InfoColor;

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextWrapped(message.message.c_str());
        ImGui::PopStyleColor();

        constexpr ImVec4 MutedMetaColor{ 0.75f, 0.75f, 0.75f, 1.0f };

        ImGui::PushStyleColor(ImGuiCol_Text, MutedMetaColor);
        ImGui::TextWrapped(fileLineString.c_str());
        ImGui::PopStyleColor();
    }

    void Engine::CreateNewProject(const std::string& path)
    {
        UnloadProject();

        if (!FileIO::IsDirectory(path))
        {
            LOG_ERROR("Could not create project. Path is not a directory: %s", path.c_str());
            return;
        }

        m_editInfo.projectSettings = {};
        m_editInfo.projectSettings.name = FileIO::GetFileName(path);
        m_editInfo.projectSettings.SaveJson(path + "/project.tfproj");

        UpdateAssetLoaderDirectory(path);

        m_level = std::make_shared<Level>();
        m_level->name = "Untitled Level";

        LOG("Project created: %s at %s", m_editInfo.projectSettings.name.c_str(), path.c_str());
    }

    void Engine::LoadProject(const std::string& settingsPath)
    {
        UnloadProject();

        if (!FileIO::FileExists(settingsPath))
        {
            LOG_ERROR("Project settings file not found: %s", settingsPath.c_str());
            return;
        }

        m_editInfo.projectSettings = {};
        m_editInfo.projectSettings.LoadJson(settingsPath);

        const std::string projectDirectory = FileIO::GetDirectory(settingsPath);
        m_editInfo.projectPath = projectDirectory;

        UpdateAssetLoaderDirectory(projectDirectory);

        if (IsValidAssetId(m_editInfo.projectSettings.laraPath))
        {
            m_lara.model = m_assets.Load<Model>(m_editInfo.projectSettings.laraPath);
            SetupLara();
        }

        if (IsValidAssetId(m_editInfo.projectSettings.defaultLevelPath))
        {
            LoadLevel(m_editInfo.projectSettings.defaultLevelPath);
        }
        else
        {
            m_level = std::make_shared<Level>();
            m_level->name = "Untitled Level";
        }

        LOG("Project loaded: %s", m_editInfo.projectSettings.name.c_str());
    }

    void Engine::SaveProject()
    {
        if (!FileIO::IsDirectory(m_editInfo.projectPath))
        {
            LOG_ERROR("Project path is not set, cannot save project settings");
            return;
        }

        m_editInfo.projectSettings.SaveJson(m_editInfo.projectPath + "/project.tfproj");

        m_assets.SaveAsset(m_level);

        // todo: save out config file
    }

    void Engine::UnloadProject()
    {
        UnloadLevel();

        m_editInfo.projectSettings = {};
        m_editInfo.projectPath = {};
        m_editInfo.assetImport.Finish();
    }

    void Engine::UpdateAssetLoaderDirectory(const std::string& directory)
    {
        m_assets.Init(directory);
    }

    void Engine::InitializeColliders()
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

        for (auto& mesh : m_level->meshColliders)
        {
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
                auto& bodies = m_physicsSystem->GetBodyInterface();
                mesh.rigidbody = CreateBody(bodies, meshShape, JPH::EMotionType::Static);
            }
            else
            {
                LOG_ERROR(result.GetError().c_str());
            }
        }
    }

    void Engine::OnObjectTransformUpdate(size_t index)
    {
    }

    void Engine::OnLaraTransformUpdate()
    {
        if (!m_lara.physics)
        {
            LOG_ERROR("Tried to upate Lara's transform but physics is not initialized");
            return;
        }

        m_lara.physics->SetPosition(GlmVec3ToJph(m_lara.transform.position));
    }

    void Engine::DrawGizmos()
    {
        if (!m_level)
        {
            return;
        }

        Level& level = *m_level;

        if (m_editInfo.selectedObject < m_level->meshes.size())
        {
            // Draw lights attached to it
            for (size_t l = 0; l < m_level->pointLights.size(); l++)
            {
                // todo: re-enable later with proper gizmo for lights
                break;
                
                for (const auto& li : m_level->meshes[m_editInfo.selectedObject].lights)
                {
                    if (li == l)
                    {
                        const PointLight& light = m_level->pointLights[l];
                        glm::vec3 position = light.position;

                        Transform lightTransform{};
                        lightTransform.position = position;
                        
                        DrawConeArrow(lightTransform.AsMatrix(), glm::vec4{1.0f, 0, 1.0f, 1.0f});
                    }
                }
            }
        }

        if (m_editInfo.isDragging && m_editInfo.selectedObject < level.meshes.size())
        {
            // So gizmos are above objects
            Graphics::Get().ClearDepthBuffer();

            Transform& objTransform = level.meshes[m_editInfo.selectedObject].transform;
            const glm::vec3 position = objTransform.position;

            constexpr float OverallScale = 0.15f;
            constexpr float SideScale = 0.2f;

            const float distanceScale = glm::length(m_camera.transform.position - position) * OverallScale;

            Transform xTransform{};
            Transform yTransform{};
            Transform zTransform{};

            xTransform.position = position;
            yTransform.position = position;
            zTransform.position = position;

            xTransform.SetEulers(0.0f, glm::radians(90.0f), 0.0f);
            yTransform.SetEulers(glm::radians(-90.0f), 0.0f, 0.0f);

            xTransform.rotation = objTransform.rotation * xTransform.rotation;
            yTransform.rotation = objTransform.rotation * yTransform.rotation;
            zTransform.rotation = objTransform.rotation * zTransform.rotation;

            const glm::vec3 arrowsScale = glm::vec3{ distanceScale * SideScale, distanceScale * SideScale, distanceScale };
            xTransform.scale = yTransform.scale = zTransform.scale = arrowsScale;

            DrawConeArrow(xTransform.AsMatrix(), glm::vec4{ 1.0f, 0, 0, 1.0f });
            DrawConeArrow(yTransform.AsMatrix(), glm::vec4{ 0, 1.0f, 0, 1.0f });
            DrawConeArrow(zTransform.AsMatrix(), glm::vec4{ 0, 0, 1.0f, 1.0f });
        }
    }

    void Engine::DrawConeArrow(const glm::mat4& transform, glm::vec4 color)
    {
        Graphics& graphics = graphics.Get();
        graphics.UseGizmoShader();

        Camera& cam = m_camera;

        graphics.SetMatrix4("model", transform);

        glm::mat4 cameraView = glm::inverse(cam.transform.AsMatrix());
        graphics.SetMatrix4("view", cameraView);

        glm::mat4 projection = glm::perspective(cam.fovY, cam.aspect, cam.near, cam.far);
        graphics.SetMatrix4("projection", projection);

        graphics.SetVec4("color", color);

        graphics.DrawMesh(m_assets.Load<Model>("Arrow")->meshes[0].gpuHandle);
    }

    void Engine::DrawDevWindows()
    {
        const bool projectLoaded = !m_editInfo.projectSettings.name.empty();

        if (!projectLoaded)
        {
            const char* windowName = "No Project Loaded";
            const ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_AlwaysAutoResize;

            // Calculate center position
            ImVec2 windowSize(m_windowWidth / 3.0f, 0); // width, height will auto-fit
            ImVec2 center = ImVec2(m_windowWidth * 0.5f, m_windowHeight * 0.5f);

            // Set next window size and position
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(center.x - windowSize.x * 0.5f, center.y - 50), ImGuiCond_Always);

            ImGui::Begin(windowName, nullptr, flags);
            ImGui::TextWrapped("No project loaded.\nCreate or open a project from the File menu.");
            ImGui::End();
        }

        // Options

        ImGui::BeginMainMenuBar();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project"))
            {
                const std::string projectPath = OpenFileDialog({}, 0, true);
                if (!projectPath.empty())
                {
                    const std::string directory = FileIO::GetDirectory(projectPath);
                    CreateNewProject(projectPath);
                }
            }
            if (ImGui::MenuItem("Open Project"))
            {
                const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                if (!filePath.empty())
                {
                    LoadProject(filePath);
                }
            }
            if (projectLoaded)
            {
                if (ImGui::MenuItem("Save Project"))
                {
                    SaveProject();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("New Level"))
                {

                }
                if (ImGui::MenuItem("Load Level"))
                {
                    const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);

                    if (!filePath.empty())
                    {
                        const AssetId id = m_assets.GetAssetId(filePath);
                        if (IsValidAssetId(id))
                        {
                            if (m_level)
                            {
                                m_renderer->DeloadLevel(*m_level);
                            }

                            LoadLevel(id);
                            m_renderer->InitializeLevel(*m_level);
                        }
                        else
                        {
                            LOG_ERROR("File is not a part of the registry");
                        }
                    }
                }
                if (m_level && ImGui::MenuItem("Save Level"))
                {
                    m_assets.SaveAsset(m_level);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                m_shouldQuit = true;
            }
            ImGui::EndMenu();
        }

        if (projectLoaded)
        {
            if (ImGui::BeginMenu("Assets"))
            {
                if (ImGui::MenuItem("Import Assets"))
                {
                    m_editInfo.importPaths.clear();
                    if (OpenMultiFileDialog(ModelFileTypes, 1, m_editInfo.importPaths))
                    {
                        for (auto it = m_editInfo.importPaths.begin(); it != m_editInfo.importPaths.end(); it++)
                        {
                            if (it->empty())
                            {
                                it = m_editInfo.importPaths.erase(it);
                            }
                        }

                        if (m_editInfo.importPaths.size() > 0)
                        {
                            m_editInfo.showImportWindow = true;
                        }
                    }
                }
                if (ImGui::MenuItem("Import GMX"))
                {
                    const std::string filePath = OpenFileDialog(AodFileTypes, NumAodFiles);
                    if (!filePath.empty())
                    {
                        GmxResult result = ImportGmx(filePath, {});

                        m_level->directionalLight.intensity = 0.0f;

                        m_level->ambientStrength = 1.0f;
                        m_level->ambientColor = SRGBToLinear(glm::vec3{ 0.1f, 0.1f, 0.1f });

                        if (result.lights.size() > 0)
                        {
                            m_level->pointLights = std::move(result.lights);
                        }

                        if (result.geometry.size() > 0)
                        {
                            m_level->models = result.geometry;

                            for (auto& model : result.geometry)
                            {
                                for (auto& mesh : model->meshes)
                                {
                                    auto& instance = m_level->meshes.emplace_back();
                                    instance.mesh = &mesh;
                                    instance.bounds = mesh.bounds;
                                    instance.modelMatrix = instance.transform.AsMatrix();

                                    const glm::vec3 lightReferencePosition = (mesh.bounds.min + mesh.bounds.max) / 2.0f;
                                    GetClosestLights(*m_level, lightReferencePosition, instance.lights);
                                }
                            }

                            m_renderer->InitializeLevel(*m_level);

                            LOG("Imported Geometry %s", filePath.c_str());
                        }

                        if (result.boxColliders.size() > 0)
                        {
                            m_level->boxColliders = std::move(result.boxColliders);
                        }

                        if (result.meshColliders.size() > 0)
                        {
                            m_level->meshColliders = std::move(result.meshColliders);
                        }

                        InitializeColliders();
                    }
                }
                if (ImGui::MenuItem("Import Texture"))
                {
                    const std::string filePath = OpenFileDialog(TextureFileTypes, NumTextureFiles);

                    if (!filePath.empty())
                    {
                        std::shared_ptr<Texture> texture = std::make_shared<Texture>();
                        if (ImportTexture(filePath, *texture))
                        {
                            const std::string outPath = OpenFileDialog({}, 0, true);
                            if (!outPath.empty())
                            {
                                const std::string absPath = outPath + "\\" + FileIO::GetFileName(filePath) + TextureFileExt;
                                m_assets.AddAsset<Texture>(texture, absPath, filePath);
                            }
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Edit Material"))
                {
                    const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                    if (!filePath.empty())
                    {
                        m_editInfo.material = m_assets.Load<Material>(filePath);
                        m_editInfo.showMaterialEditor = true;
                    }
                }
                if (ImGui::MenuItem("Edit Animation"))
                {
                    const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                    if (!filePath.empty())
                    {
                        m_editInfo.animation = m_assets.Load<Animation>(filePath);
                        m_editInfo.showAnimEditor = m_editInfo.animation != nullptr;
                    }
                }
                if (m_level)
                {
                    ImGui::Separator();
                    if (ImGui::MenuItem("Instantiate Model"))
                    {
                        const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);

                        if (!filePath.empty())
                        {
                            std::shared_ptr<Model> model = m_assets.Load<Model>(filePath);
                            // todo: check if model is already in level

                            if (model)
                            {
                                m_level->models.emplace_back(model);

                                for (auto& mesh : model->meshes)
                                {
                                    auto& instance = m_level->meshes.emplace_back();
                                    instance.mesh = &mesh;
                                    instance.bounds = mesh.bounds;
                                    instance.modelMatrix = instance.transform.AsMatrix();
                                    const glm::vec3 lightReferencePosition = (mesh.bounds.min + mesh.bounds.max) / 2.0f;
                                    GetClosestLights(*m_level, lightReferencePosition, instance.lights);
                                }

                                m_renderer->InitializeLevel(*m_level);
                            }
                            else
                            {
                                LOG_ERROR("Could not load model %s", filePath.c_str());
                            }
                        }
                    }
                    if (ImGui::MenuItem("Create Cube"))
                    {
                        auto cubeModel = m_assets.Load<Model>("Cube");
                        m_level->models.emplace_back(cubeModel);

                        auto& obj = m_level->meshes.emplace_back();
                        obj.name = "Cube";
                        obj.mesh = &cubeModel->meshes[0];
                        
                        UpdateBounds(obj);

                        m_renderer->InitializeLevel(*m_level);
                    }
                }
                if (ImGui::MenuItem("Asset Registry"))
                {
                    m_editInfo.showRegistryWindow = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Lighting Settings"))
            {
                m_editInfo.showLightingWindow = true;
            }

            if (ImGui::MenuItem("Lara Settings"))
            {
                m_editInfo.showLaraWindow = true;
            }
        }

        ImGui::EndMainMenuBar();

        // Items from menu bar

        if (m_editInfo.showAnimEditor)
        {
            DrawAnimEditor();
        }

        if (m_editInfo.showMaterialEditor)
        {
            DrawMaterialEditor();
        }

        if (m_editInfo.showLightingWindow)
        {
            DrawLightingWindow();
        }

        if (m_editInfo.showImportWindow)
        {
            DrawImportWindow();
        }

        if (m_editInfo.showLaraWindow)
        {
            DrawLaraWindow();
        }

        if (m_editInfo.showRegistryWindow)
        {
            DrawRegistryWindow();
        }

        // Inspector

        DrawInspector();

        // Debug Messages

        const ImGuiWindowFlags statsFlags =
            ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize;

        ImGui::Begin("Log", 0, statsFlags);
        ImGui::SetWindowSize({ 350.0f, m_windowHeight - ImGui::GetTextLineHeightWithSpacing() - 30.0f });
        ImGui::SetWindowPos({ 0.0f, ImGui::GetTextLineHeightWithSpacing()});
        Debug::MessageLoop(PrintDebugMessage);
        ImGui::End();

        // Stats

        const Transform& camTransform = m_camera.transform;
        const glm::vec3 camEulers = m_camera.transform.EulerRotation();

        ImGui::Begin("Stats", 0, statsFlags);
        ImGui::SetWindowSize({ static_cast<float>(m_windowWidth), 30.0f });
        ImGui::SetWindowPos({ 0.0f, m_windowHeight - 30.0f });
        ImGui::Text("FPS: %i - Camera Pos: (%f, %f, %f) - Camera Rot: (%f, %f, %f)", 
            m_fps, 
            camTransform.position.x, 
            camTransform.position.y, 
            camTransform.position.z,
            glm::degrees(camEulers.x),
            glm::degrees(camEulers.y),
            glm::degrees(camEulers.z));
        ImGui::End();
    }

    void Engine::DrawLightingWindow()
    {
        ImGui::Begin("Lighting");
        ImGui::SeparatorText("Ambient");
        ImGui::InputFloat3("Ambient Color", &m_level->ambientColor.x);
        ImGui::InputFloat("Ambient Strength", &m_level->ambientStrength);
        ImGui::SeparatorText("Directional Light");
        ImGui::InputFloat3("Directional Color", &m_level->directionalLight.color.x);
        ImGui::InputFloat3("Directional Forward", &m_level->directionalLight.dir.x);
        ImGui::InputFloat("Directional Intensity", &m_level->directionalLight.intensity);
        if (ImGui::Button("Close"))
        {
            m_editInfo.showLightingWindow = false;
        }
        ImGui::End();
    }

    void Engine::DrawMaterialEditor()
    {
        ImGui::Begin("Material Editor");
        if (!m_editInfo.material)
        {
            ImGui::Text("No material opened");
        }
        else
        {
            ImGui::Text("Name: %s", m_editInfo.material->name.c_str());
            if (m_editInfo.material->diffuse)
            {
                ImGui::Text("Diffuse: %s", m_editInfo.material->diffuse->name.c_str());
            }
            else
            {
                ImGui::Text("Diffuse: Null");
            }
            if (ImGui::Button("Set Diffuse"))
            {
                const std::string texturePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                if (!texturePath.empty())
                {
                    m_editInfo.material->diffuse = m_assets.Load<Texture>(texturePath);
                    if (m_editInfo.material->diffuse)
                    {
                        m_editInfo.material->AddFlag(MATERIAL_FLAG_DIFFUSE);
                    }
                    else
                    {
                        m_editInfo.material->RemoveFlag(MATERIAL_FLAG_DIFFUSE);
                    }
                }
            }
            bool isTransparent = m_editInfo.material->TestFlag(MATERIAL_FLAG_TRANSPARENT);
            if (ImGui::Checkbox("Is Transparent", &isTransparent))
            {
                if (isTransparent)
                {
                    m_editInfo.material->AddFlag(MATERIAL_FLAG_TRANSPARENT);
                }
                else
                {
                    m_editInfo.material->RemoveFlag(MATERIAL_FLAG_TRANSPARENT);
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                m_assets.SaveAsset(m_editInfo.material);
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Close"))
        {
            m_editInfo.material = nullptr;
            m_editInfo.showMaterialEditor = false;
        }
        ImGui::End();
    }

    void Engine::DrawAnimEditor()
    {
        ImGui::SetNextWindowSize({ 480, 520 }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Anim Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (!m_editInfo.animation)
        {
            ImGui::Text("No anim selected");
            ImGui::End();
            return;
        }

        auto anim = m_editInfo.animation;

        ImGui::InputText("Name: %s", &anim->name);

        ImGui::SeparatorText("Settings");
        ImGui::Checkbox("Root Motion", &m_editInfo.animation->hasRootMotion);

        ImGui::SeparatorText("Info");
        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, 180.0f);
        ImGui::Text("FPS:");
        ImGui::Text("Length (frames):");
        ImGui::Text("Length (s):");
        ImGui::Text("Key Count:");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        ImGui::Text("%.2f", anim->framerate);
        ImGui::Text("%.2f", anim->length);
        ImGui::Text("%.2f", anim->length / anim->framerate);
        ImGui::Text("%i", anim->keys.size());

        ImGui::Columns(1);
        ImGui::SeparatorText("Events");
        for (size_t i = 0; i < anim->events.size(); i++)
        {
            EventKey& key = anim->events[i];

            ImGui::PushID(i);
            if (ImGui::BeginCombo("Type", AnimEventToString((AnimEvent)key.value).c_str()))
            {
                for (uint8_t eventType = 0; eventType < ANIM_EVENT_COUNT; eventType++)
                {
                    bool selected = key.value == eventType;
                    if (ImGui::Selectable(AnimEventToString((AnimEvent)eventType).c_str(), &selected))
                        key.value = (AnimEvent)eventType;
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::InputFloat("Time", &key.time, 0.0f, 0.0f, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                anim->events.erase(anim->events.begin() + i);
                ImGui::PopID();
                break; // to avoid issues with changed indices
            }
            ImGui::PopID();
        }

        if (ImGui::Button("Add Event"))
        {
            anim->events.emplace_back();
        }

        ImGui::Separator();

        if (ImGui::Button("Save"))
        {
            std::sort(anim->events.begin(), anim->events.end(), [](EventKey& key1, EventKey& key2) { return key1.time < key2.time; });
            m_assets.SaveAsset(anim);
        }

        ImGui::SameLine();

        if (m_lara.model && m_lara.animPlayer.IsValid() && ImGui::Button("Preview"))
        {
            m_lara.animPlayer.Play(anim, true);
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            m_editInfo.animation = nullptr;
            m_editInfo.showAnimEditor = false;
        }

        ImGui::End();
    }

    void Engine::DrawInspector()
    {
        const ImGuiWindowFlags inspectorFlags =
            ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoTitleBar;

        ImGui::Begin("Inspector", 0, inspectorFlags);
        ImGui::SetWindowSize({ 350.0f, m_windowHeight - ImGui::GetTextLineHeightWithSpacing() - 30.0f });
        ImGui::SetWindowPos({ m_windowWidth - 350.0f, ImGui::GetTextLineHeightWithSpacing() });
        ImGui::BeginTabBar("Tabs");
        if (ImGui::BeginTabItem("Inspector"))
        {
            if (!m_level)
            {
                ImGui::Text("No level loaded");
            }
            else
            {
                static glm::vec3 s_eulerRotation{};
                if (ImGui::BeginListBox("Mesh Instances"))
                {
                    for (size_t i = 0; i < m_level->meshes.size(); i++)
                    {
                        ImGui::PushID(i);
                        auto& staticObj = m_level->meshes[i];
                        bool selected = i == m_editInfo.selectedObject;
                        const char* name = staticObj.mesh->name.size() > 0 ? staticObj.mesh->name.c_str() : "#UNNAMED!";
                        if (ImGui::Selectable(name, &selected))
                        {
                            if (m_editInfo.selectedObject == i)
                            {
                                m_editInfo.isDragging = false;
                                s_eulerRotation = {};
                                m_editInfo.selectedObject = SIZE_MAX;
                            }
                            else
                            {
                                m_editInfo.selectedObject = i;
                                s_eulerRotation = staticObj.transform.EulerRotation();
                                m_editInfo.isDragging = true;
                            }
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndListBox();
                }
                ImGui::SeparatorText("Details");
                if (m_editInfo.selectedObject < m_level->meshes.size())
                {
                    MeshInstance& obj = m_level->meshes[m_editInfo.selectedObject];
                    ImGui::InputText("Name", &obj.name);
                    bool transformUpdated = false;
                    transformUpdated |= ImGui::InputFloat3("Position", &obj.transform.position.x);
                    if (ImGui::InputFloat3("Rotation", &s_eulerRotation.x))
                    {
                        transformUpdated = true;
                        obj.transform.SetEulers(glm::radians(s_eulerRotation.x), glm::radians(s_eulerRotation.y), glm::radians(s_eulerRotation.z));
                    }
                    transformUpdated |= ImGui::InputFloat3("Scale", &obj.transform.scale.x);
                    if (transformUpdated)
                    {
                        OnObjectTransformUpdate(m_editInfo.selectedObject);
                    }
                    ImGui::SeparatorText("Mesh");
                    ImGui::Text("Mesh: %s", obj.mesh ? obj.mesh->name.c_str() : "Null");
                    if (obj.mesh)
                    {
                        ImGui::Text("Material Name: %s", obj.mesh->name.c_str());
                    }
                    ImGui::SeparatorText("Lights");
                    ImGui::Text("%i, %i, %i, %i, %i, %i, %i, %i",
                        obj.lights[0],
                        obj.lights[1],
                        obj.lights[2],
                        obj.lights[3],
                        obj.lights[4],
                        obj.lights[5],
                        obj.lights[6],
                        obj.lights[7]);
                    if (ImGui::Button("Delete Object"))
                    {
                        DeleteLevelObject(m_editInfo.selectedObject);
                    }
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting"))
        {
            if (ImGui::BeginListBox("Point Lights"))
            {
                for (size_t l = 0; l < m_level->pointLights.size(); l++)
                {
                    if (m_editInfo.selectedObject < m_level->meshes.size())
                    {
                        bool found = false;
                        for (auto& light : m_level->meshes[m_editInfo.selectedObject].lights)
                        {
                            if (light == l)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            continue;
                        }
                    }

                    bool selected = m_editInfo.selectedPointLight == l;
                    if (ImGui::Selectable(std::to_string(l).c_str(), &selected))
                    {
                        m_editInfo.selectedPointLight = l;
                    }
                }
                ImGui::EndListBox();
            }

            if (m_editInfo.selectedPointLight < m_level->pointLights.size())
            {
                auto& light = m_level->pointLights[m_editInfo.selectedPointLight];
                ImGui::InputFloat3("Position", &light.position.x);
                ImGui::InputFloat3("Color", &light.color.r);
                ImGui::InputFloat("Intensity", &light.intensity);
                ImGui::InputFloat("Inner Radius", &light.innerRadius);
                ImGui::InputFloat("Outer Radius", &light.outerRadius);
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::End();
    }

    void Engine::DrawImportWindow()
    {
        static ImportSettings settings{};

        ImGui::Begin("Importer");

        bool importModel = settings.importModel;
        bool importSkeleton = settings.importSkeleton;
        bool importAnimation = settings.importAnimation;

        if (ImGui::Checkbox("Import Model", &importModel))
        {
            settings.importModel = importModel;
        }

        if (ImGui::Checkbox("Import Skeleton", &importSkeleton))
        {
            settings.importSkeleton = importSkeleton;
        }

        if (ImGui::Checkbox("Import Animation", &importAnimation))
        {
            settings.importAnimation = importAnimation;
        }

        ImGui::Separator();

        if (settings.importAnimation && !settings.importSkeleton)
        {
            if (ImGui::Button("Select Skeleton"))
            {
                const std::string skeletonPath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                const AssetId id = m_assets.GetAssetId(skeletonPath);
                if (IsValidAssetId(id))
                {
                    settings.existingSkeleton = m_assets.Load<Skeleton>(id);
                }
            }
            if (settings.existingSkeleton)
            {
                ImGui::Text("Using Skeleton: %s", settings.existingSkeleton->name.c_str());
            }
            else
            {
                ImGui::Text("No Skeleton Selected");
            }
        }

        ImGui::Separator();

        ImGui::InputFloat("Scale", &settings.scale, 0.0f, 0.0f, "%.6f");

        ImGui::Separator();

        if (ImGui::Button("Import"))
        {
            std::string basePath = OpenFileDialog({}, 0, true);
            if (!basePath.empty())
            {
                if (basePath.find('.') != std::string::npos)
                {
                    basePath = FileIO::GetBasePath(basePath); // Don't want file
                }

                if (!basePath.empty() && (basePath[basePath.size() - 1] != '/' || basePath[basePath.size() - 1] != '\\'))
                {
                    basePath.append("\\");
                }
            }

            if (!basePath.empty())
            {
                for (auto& filePath : m_editInfo.importPaths)
                {
                    m_editInfo.assetImport.Start(filePath);

                    const std::string fileName = FileIO::GetFileName(filePath);
                    
                    if (settings.importModel)
                    {
                        settings.modelPath = basePath;
                        settings.modelPath.append(fileName);
                        settings.modelPath.append(ModelFileExt);
                    }

                    if (settings.importSkeleton)
                    {
                        settings.skeletonPath = basePath;
                        settings.skeletonPath.append(fileName);
                        settings.skeletonPath.append(SkeletonFileExt);
                    }

                    if (settings.importAnimation)
                    {
                        settings.animationPath = basePath;
                        settings.animationPath.append(fileName);
                        settings.animationPath.append(AnimFileExt);
                    }

                    if (m_editInfo.assetImport.Start(filePath))
                    {
                        auto result = m_editInfo.assetImport.Import(settings);

                        if (result.model && settings.importModel)
                        {
                            for (auto& mesh : result.model->meshes)
                            {
                                if (!mesh.material)
                                {
                                    continue;
                                }

                                if (mesh.material->diffuse)
                                {
                                    m_assets.AddAsset(mesh.material->diffuse, mesh.material->diffuse->name, filePath);
                                }

                                m_assets.AddAsset(mesh.material, mesh.material->name, filePath);
                            }

                            m_assets.AddAsset(result.model, settings.modelPath, filePath);
                        }

                        if (result.skeleton && settings.importSkeleton)
                        {
                            m_assets.AddAsset(result.skeleton, settings.skeletonPath, filePath);
                        }

                        if (result.animation && settings.importAnimation)
                        {
                            m_assets.AddAsset(result.animation, settings.animationPath, filePath);
                        }
                    }
                    else
                    {
                        LOG_ERROR("Failed to start import process for %s", filePath.c_str());
                    }
                }

                LOG("Imported Finished");
                m_editInfo.assetImport.Finish();
                m_editInfo.showImportWindow = false;

                m_assets.Save();
            }
            else
            {
                LOG_ERROR("Could not import to empty file path");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            m_editInfo.showImportWindow = false;
        }

        ImGui::End();
    }

    void Engine::DrawLaraWindow()
    {
        ImGui::SetNextWindowSize({ 520.0f, 700.0f }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Lara Settings", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::InputFloat3("Position", &m_lara.transform.position.x))
        {
            OnLaraTransformUpdate();
        }
        glm::vec3 eulers = m_lara.transform.EulerRotation();
        if (ImGui::InputFloat3("Rotation", &eulers.x))
        {
            m_lara.transform.SetEulers(glm::radians(eulers.x), glm::radians(eulers.y), glm::radians(eulers.z));
            OnLaraTransformUpdate();
        }
        if (ImGui::InputFloat3("Scale", &m_lara.transform.scale.x))
        {
            OnLaraTransformUpdate();
        }

        ImGui::SeparatorText("Model");
        ImGui::Text("Asset: %s", m_lara.model ? m_lara.model->name.c_str() : "Empty");

        if (ImGui::Button("Set Lara Model"))
        {
            const std::string& filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
            if (!filePath.empty())
            {
                m_editInfo.projectSettings.laraPath = m_assets.GetAssetId(filePath);
                SetupLara();
            }
        }

        if (m_lara.model)
        {
            ImGui::SeparatorText("Meshes");
            ImGui::BeginChild("MeshList", { 500, 300 });
            for (size_t i = 0; i < m_lara.model->meshes.size(); i++)
            {
                auto& mesh = m_lara.model->meshes[i];
                ImGui::PushID(i);
                ImGui::Text("Mesh %i: %s", i, mesh.name.c_str());
                ImGui::Text("Material: %s", mesh.material ? mesh.material->name.c_str() : "Empty");

                if (ImGui::Button("Set Material"))
                {
                    const std::string& filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                    if (!filePath.empty())
                    {
                        mesh.material = m_assets.Load<Material>(filePath);
                    }
                }
                ImGui::Checkbox("Is Active", &mesh.isActive);
                ImGui::PopID();
                ImGui::Separator();
            }
            ImGui::EndChild();

            ImGui::SeparatorText("Skeleton");
            const auto& skel = m_lara.model->skeleton;
            ImGui::BeginChild("SkeletonBones", { 500, 300 });
            for (size_t b = 0; b < skel->bones.size(); b++)
            {
                auto& bone = skel->bones[b];

                glm::vec3 bonePosition{};
                glm::vec3 boneScale{};
                glm::quat boneRotation{};
                glm::vec3 skew{}; // ignored
                glm::vec4 perspective{}; // ignored
                glm::decompose(bone.transform, boneScale, boneRotation, bonePosition, skew, perspective);

                glm::vec3 offsetPosition{};
                glm::vec3 offsetScale{};
                glm::quat offsetRotation{};
                glm::decompose(bone.offset, offsetScale, offsetRotation, offsetPosition, skew, perspective);

                ImGui::PushID(b);
                ImGui::Text("Bone %zu: %s (parent: %i)", b, bone.name.c_str(), bone.parent);

                ImGui::Text("Transform");
                ImGui::InputFloat3("Pos", &bonePosition.x);
                glm::vec3 eulersBone = glm::degrees(glm::eulerAngles(boneRotation));
                ImGui::InputFloat3("Rot", &eulersBone.x);
                ImGui::InputFloat3("Scale", &boneScale.x);

                ImGui::Text("Offset");
                ImGui::InputFloat3("Pos##off", &offsetPosition.x);
                glm::vec3 eulersOffset = glm::degrees(glm::eulerAngles(offsetRotation));
                ImGui::InputFloat3("Rot##off", &eulersOffset.x);
                ImGui::InputFloat3("Scale##off", &offsetScale.x);

                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        else
        {
            ImGui::Text("No model loaded.");
        }

        if (m_lara.model && ImGui::Button("Save Model"))
        {
            m_assets.SaveAsset(m_lara.model);
            ImGui::SameLine();
        }

        if (ImGui::Button("Close"))
        {
            m_editInfo.showLaraWindow = false;
        }
        ImGui::End();
    }

    void Engine::DrawRegistryWindow()
    {
        ImGui::SetNextWindowSize({ 700.0f, 500.0f }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Asset Registry");

        ImGui::Text("Total Assets: %zu", m_assets.m_assets.size());
        ImGui::Separator();

        ImGui::Columns(4, "assetColumns");
        ImGui::Text("ID"); ImGui::NextColumn();
        ImGui::Text("Type"); ImGui::NextColumn();
        ImGui::Text("Asset Path"); ImGui::NextColumn();
        ImGui::Text("Source Path"); ImGui::NextColumn();
        ImGui::Separator();

        for (const auto& [id, meta] : m_assets.m_assets)
        {
            ImGui::Text("%zu", id); 
            ImGui::NextColumn();

            // Convert AssetType to string
            const char* typeStr = "Unknown";
            switch (meta.type)
            {
                case ASSET_TYPE_MODEL:
                    typeStr = "Model";
                    break;
                case ASSET_TYPE_TEXTURE:
                    typeStr = "Texture";
                    break;
                case ASSET_TYPE_MATERIAL:
                    typeStr = "Material";
                    break;
                case ASSET_TYPE_ANIMATION:
                    typeStr = "Animation";
                    break;
                case ASSET_TYPE_SKELETON:
                    typeStr = "Skeleton";
                    break;
                case ASSET_TYPE_LEVEL:
                    typeStr = "Level";
                    break;
                default:
                    break;
            }
            ImGui::TextUnformatted(typeStr); 
            ImGui::NextColumn();

            ImGui::TextUnformatted(meta.assetPath.c_str()); 
            ImGui::NextColumn();

            ImGui::TextUnformatted(meta.sourcePath.c_str());
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        if (ImGui::Button("Save"))
        {
            m_assets.Save();
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            m_editInfo.showRegistryWindow = false;
        }

        ImGui::End();
    }
#endif
}
