#include "Engine/Editor.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui.h>
#include <limits>
#include <misc/cpp/imgui_stdlib.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "Core/Graphics/Color.h"
#include "Core/IO/DevIO.h"
#include "Core/IO/FileIO.h"
#include "Core/Maths/Maths.h"
#include "Engine/Animation/Animation.h"
#include "Engine/Animation/Skeleton.h"
#include "Engine/Audio/Sound.h"
#include "Engine/Assets/GmxImport.h"
#include "Engine/Assets/ModelImporter.h"
#include "Engine/Assets/TextureImport.h"
#include "Engine/Engine.h"
#include "Engine/Player/Input.h"
#include "Engine/Rendering/JoltDebugRenderer.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/Texture.h"

namespace TombForge
{
    namespace
    {
        constexpr char const* SkeletonFileExt{ ".tfskel" };
        constexpr char const* AnimFileExt{ ".tfanim" };
        constexpr char const* ModelFileExt{ ".tfmod" };
        constexpr char const* TextureFileExt{ ".tftex" };
        constexpr char const* MaterialFileExt{ ".tfmat" };
        constexpr char const* LevelFileExt{ ".tflev" };
        constexpr char const* ProjectFileExt{ ".tfproj" };

        constexpr unsigned int NumTombSlateFiles{ 1 };
        constexpr unsigned int NumImportFiles{ 1 };
        constexpr unsigned int NumAodFiles{ 1 };
        constexpr unsigned int NumTextureFiles{ 1 };

        constexpr float ScrollIncreaseRate{ 0.5f };
        constexpr float ScrollMaxSpeed{ 20.0f };
        constexpr float ScrollMinSpeed{ 0.1f };

        constexpr COMDLG_FILTERSPEC TombSlateFileTypes[] =
        {
            { L"TombForge Asset", L"*.tombs;*.tfskel;*.tfanim;*.tfmod;*.tftex;*.tfmat;*.tflev;*.tfproj;*.wav;*.ogg" }
        };

        constexpr COMDLG_FILTERSPEC ModelFileTypes[] =
        {
            { L"Supported Model Types", L"*.fbx;*.obj;*.3ds;*.dae;*.gltf;*.glb" }
        };

        constexpr COMDLG_FILTERSPEC AodFileTypes[] =
        {
            { L"AOD Level File", L"*.gmx;*.gmx.clz" }
        };

        constexpr COMDLG_FILTERSPEC TextureFileTypes[] =
        {
            { L"Supported Texture Types", L"*.jpg;*.jpeg;*.tga;*.png;*.bmp;*.tif" }
        };

        constexpr COMDLG_FILTERSPEC SoundFileTypes[] =
        {
            { L"Supported Sound Types", L"*.wav;*.ogg" }
        };

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
    }

    Editor::Editor(EngineContext& ctx)
        : m_ctx(ctx)
        , m_physicsDebugRenderer(new JoltDebugRenderer())
    {
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(ctx.window, true);
        ImGui_ImplOpenGL3_Init();

        Input::RegisterKeyCallback([this](int key, int scancode, int action, int mods)
            {
                this->HandleKey(key, scancode, action, mods);
            });

        Input::RegisterMouseScrollCallback([this](float scroll)
            {
                this->HandleScroll(scroll);
            });

        Input::RegisterMouseButtonCallback([this](int button, int action, int mods)
            {
                this->HandleMouseButton(button, action, mods);
            });

        m_ctx.physicsInterface.SetDebugRenderer(m_physicsDebugRenderer);

        m_ctx.isFreeCamera = true;
        m_ctx.isPaused = true;
        m_isEditMode = true;
    }

    Editor::~Editor()
    {
        m_ctx.physicsInterface.SetDebugRenderer(nullptr);
        delete m_physicsDebugRenderer;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void Editor::Update()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        DrawDebugShapes();

        if (m_isEditMode)
        {
            DrawGizmos();
            DrawEditorUI();
        }

        glDisable(GL_FRAMEBUFFER_SRGB); // ImGui does not use linear space
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glEnable(GL_FRAMEBUFFER_SRGB);

        ImGui::EndFrame();
    }

    void Editor::DrawGizmos()
    {
        if (!m_ctx.level)
        {
            return;
        }

        auto& level = *m_ctx.level;

        if (m_selectedObject < level.meshes.size())
        {
            auto& mesh = level.meshes[m_selectedObject];

            for (size_t l = 0; l < level.pointLights.size(); l++)
            {
                for (const auto& li : mesh.lights)
                {
                    if (li == l)
                    {
                        const PointLight& light = level.pointLights[l];
                        glm::vec3 position = light.position;

                        Transform lightTransform{};
                        lightTransform.position = position;

                        // todo: add draw for light sprite
                    }
                }
            }

            Graphics::Get().SetDepthTest(false); // Keep gizmos on top of everything

            Transform& objTransform = mesh.transform;
            const glm::vec3 position = objTransform.position;

            constexpr float OverallScale = 0.15f;
            constexpr float SideScale = 0.2f;

            const float distanceScale = glm::length(m_ctx.camera.transform.position - position) * OverallScale;

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

            Graphics::Get().SetDepthTest(true);
        }
    }

    void Editor::DrawDebugShapes()
    {
        if (m_showColliders)
        {
            if (m_ctx.lara.physics)
            {
                m_ctx.lara.physics->GetShape()->Draw(
                    m_physicsDebugRenderer,
                    m_ctx.lara.physics->GetCenterOfMassTransform(),
                    JPH::Vec3::sReplicate(1.0f),
                    JPH::Color::sWhite,
                    false, true);
            }

            for (auto& obj : m_ctx.level->boxColliders)
            {
                JPH::BodyInterface& bodies = m_ctx.physics.system->GetBodyInterface();
                JPH::ShapeRefC shape = bodies.GetShape(obj.rigidbody);
                if (shape)
                {
                    shape->Draw(m_physicsDebugRenderer, bodies.GetCenterOfMassTransform(obj.rigidbody), JPH::Vec3{ 1.0f, 1.0f, 1.0f }, JPH::Color{ 0,255,0,255 }, false, true);
                }
            }

            for (auto& obj : m_ctx.level->meshColliders)
            {
                JPH::BodyInterface& bodies = m_ctx.physics.system->GetBodyInterface();
                JPH::ShapeRefC shape = bodies.GetShape(obj.rigidbody);
                if (shape)
                {
                    shape->Draw(m_physicsDebugRenderer, bodies.GetCenterOfMassTransform(obj.rigidbody), JPH::Vec3{ 1.0f, 1.0f, 1.0f }, JPH::Color{ 0,255,0,255 }, false, true);
                }
            }
        }

        if (m_showMeshWireframe || m_drawNormals)
        {
            if (m_ctx.level->meshes.size() > 0 && m_selectedObject < m_ctx.level->meshes.size())
            {
                auto& obj = m_ctx.level->meshes[m_selectedObject];
                auto& mesh = m_ctx.level->models[obj.model]->meshes[obj.mesh];
                m_ctx.renderer->RenderWireframe(mesh, obj.transform, m_ctx.camera);
            }
        }

        if (m_drawOctree)
        {
            m_ctx.renderer->DrawOctree(glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, m_ctx.camera);

            if (m_selectedObject < m_ctx.level->meshes.size())
            {
                m_ctx.renderer->DrawBox(
                    m_ctx.level->meshes[m_selectedObject].bounds,
                    glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
                    m_ctx.camera);
            }
        }

        if (m_drawNormals)
        {
            if (m_ctx.level->meshes.size() > 0 && m_selectedObject < m_ctx.level->meshes.size())
            {
                auto& obj = m_ctx.level->meshes[m_selectedObject];
                auto& mesh = m_ctx.level->models[obj.model]->meshes[obj.mesh];

                m_ctx.renderer->DrawBox(
                    mesh.bounds,
                    glm::vec4{ 0.5f, 0.5f, 1.0f, 1.0f },
                    m_ctx.camera);

                for (Vertex& v : mesh.vertices)
                {
                    glm::vec3 start = obj.transform.position + (obj.transform.rotation * (v.position * obj.transform.scale));
                    glm::vec3 end = start + (obj.transform.rotation * (v.normal * 0.1f));
                    m_physicsDebugRenderer->DrawColoredLine( start, end, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f } );

                    start = obj.transform.position + (obj.transform.rotation * (v.position * obj.transform.scale));
                    end = start + (obj.transform.rotation * (v.tangent * 0.1f));
                    m_physicsDebugRenderer->DrawColoredLine(start, end, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });

                    start = obj.transform.position + (obj.transform.rotation * (v.position * obj.transform.scale));
                    end = start + (obj.transform.rotation * (v.bitangent * 0.1f));
                    m_physicsDebugRenderer->DrawColoredLine(start, end, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
                }
            }
        }

        m_physicsDebugRenderer->SubmitLines(
            m_ctx.camera.transform,
            m_ctx.camera.fovY,
            m_ctx.camera.aspect,
            m_ctx.camera.near,
            m_ctx.camera.far);

        m_physicsDebugRenderer->ClearLines();
    }

    void Editor::DrawConeArrow(const glm::mat4& transform, glm::vec4 color)
    {
        // todo: look at having some kind of editor-only renderer to do this
        Graphics& graphics = graphics.Get();
        graphics.UseGizmoShader();

        Camera& cam = m_ctx.camera;

        graphics.SetMatrix4("model", transform);

        glm::mat4 cameraView = glm::inverse(cam.transform.AsMatrix());
        graphics.SetMatrix4("view", cameraView);

        glm::mat4 projection = glm::perspective(cam.fovY, cam.aspect, cam.near, cam.far);
        graphics.SetMatrix4("projection", projection);

        graphics.SetVec4("color", color);

        graphics.DrawMesh(m_ctx.assetRegistry.Load<Model>("Arrow")->meshes[0].gpuHandle);
    }

    void Editor::DrawEditorUI()
    {
        const bool projectLoaded = !m_project.name.empty();

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
            ImVec2 windowSize(m_ctx.windowWidth / 3.0f, 0); // width, height will auto-fit
            ImVec2 center = ImVec2(m_ctx.windowWidth * 0.5f, m_ctx.windowHeight * 0.5f);

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
                    NewProject(projectPath);
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
                        const AssetId id = m_ctx.assetRegistry.GetAssetId(filePath);
                        if (IsValidAssetId(id))
                        {
                            UnloadLevel(m_ctx);
                            LoadLevel(m_ctx, id);
                        }
                        else
                        {
                            LOG_ERROR("File is not a part of the registry");
                        }
                    }
                }
                if (m_ctx.level && ImGui::MenuItem("Save Level"))
                {
                    m_ctx.assetRegistry.SaveAsset(m_ctx.level);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                m_ctx.shouldQuit = true;
            }
            ImGui::EndMenu();
        }

        if (projectLoaded)
        {
            if (ImGui::BeginMenu("Assets"))
            {
                if (ImGui::MenuItem("Import Assets"))
                {
                    m_importPaths.clear();
                    if (OpenMultiFileDialog(ModelFileTypes, 1, m_importPaths))
                    {
                        for (auto it = m_importPaths.begin(); it != m_importPaths.end(); it++)
                        {
                            if (it->empty())
                            {
                                it = m_importPaths.erase(it);
                            }
                        }

                        if (m_importPaths.size() > 0)
                        {
                            m_showImportWindow = true;
                        }
                    }
                }
                if (ImGui::MenuItem("Import GMX"))
                {
                    const std::string filePath = OpenFileDialog(AodFileTypes, NumAodFiles);
                    if (!filePath.empty())
                    {
                        GmxResult result = ImportGmx(filePath, {});

                        m_ctx.level->directionalLight.intensity = 0.0f;
                        m_ctx.level->ambientStrength = 1.0f;
                        m_ctx.level->ambientColor = SRGBToLinear(glm::vec3{ 0.1f, 0.1f, 0.1f });
                        m_ctx.level->pointLights = std::move(result.lights);
                        m_ctx.level->models = result.geometry;

                        for (auto& model : result.geometry)
                        {
                            m_ctx.level->models.emplace_back(model);

                            for (size_t m = 0; m < model->meshes.size(); m++)
                            {
                                auto& instance = m_ctx.level->meshes.emplace_back();
                                instance.model = static_cast<uint32_t>(m_ctx.level->models.size() - 1);
                                instance.mesh = m;
                                instance.bounds = model->meshes[m].bounds;
                                instance.modelMatrix = instance.transform.AsMatrix();

                                const glm::vec3 lightReferencePosition = (instance.bounds.min + instance.bounds.max) / 2.0f;
                                GetClosestLights(*m_ctx.level, lightReferencePosition, instance.lights);
                            }
                        }

                        m_ctx.renderer->InitializeLevel(*m_ctx.level);

                        SetAndInitColliders(m_ctx, std::move(result.boxColliders));
                        SetAndInitColliders(m_ctx, std::move(result.meshColliders));

                        LOG("Imported GMX file %s", filePath.c_str());
                    }
                }
                if (ImGui::MenuItem("Import Texture"))
                {
                    std::vector<std::string> m_importPaths{};
                    if (OpenMultiFileDialog(TextureFileTypes, 1, m_importPaths))
                    {
                        for (auto it = m_importPaths.begin(); it != m_importPaths.end(); it++)
                        {
                            if (!it->empty())
                            {
                                std::shared_ptr<Texture> texture = std::make_shared<Texture>();
                                if (ImportTexture(*it, *texture))
                                {
                                    const std::string outPath = OpenFileDialog({}, 0, true);
                                    if (!outPath.empty())
                                    {
                                        const std::string absPath = outPath + "\\" + FileIO::GetFileName(*it) + TextureFileExt;
                                        m_ctx.assetRegistry.AddAsset<Texture>(texture, absPath, *it);
                                        m_ctx.assetRegistry.Save();
                                    }
                                }
                            }
                        }
                    }
                }
                if (ImGui::MenuItem("Import Sound"))
                {
                    std::vector<std::string> importPaths{};
                    if (OpenMultiFileDialog(SoundFileTypes, 1, importPaths))
                    {
                        const std::string& outFolder = OpenFileDialog({}, 0, true);
                        if (!outFolder.empty())
                        {
                            for (auto it = importPaths.begin(); it != importPaths.end(); it++)
                            {
                                if (it->empty())
                                {
                                    continue;
                                }

                                if (!FileIO::IsExtension(*it, ".wav") && !FileIO::IsExtension(*it, ".ogg"))
                                {
                                    LOG_ERROR("Unsupported sound format: %s. Use OGG/Wav.", it->c_str());
                                    continue;
                                }

                                const std::string& outFile = outFolder + FileIO::Separator + FileIO::GetFileName(*it, true);
                                FileIO::CopyFile(*it, outFile);

                                std::shared_ptr<Sound> sound = std::make_shared<Sound>();
                                m_ctx.assetRegistry.AddAsset<Sound>(sound, outFile, *it);
                                m_ctx.assetRegistry.Save();

                                // AssetRegistry loads the data, not us beforehand
                                sound = m_ctx.assetRegistry.Load<Sound>(outFile);
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
                        m_material = m_ctx.assetRegistry.Load<Material>(filePath);
                        m_showMaterialEditor = m_material != nullptr;
                    }
                }
                if (ImGui::MenuItem("Edit Animation"))
                {
                    const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                    if (!filePath.empty())
                    {
                        m_animation = m_ctx.assetRegistry.Load<Animation>(filePath);
                        m_showAnimEditor = m_animation != nullptr;
                    }
                }
                if (ImGui::MenuItem("Edit Model"))
                {
                    const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                    if (!filePath.empty())
                    {
                        m_model = m_ctx.assetRegistry.Load<Model>(filePath);
                        m_showModelWindow = m_model != nullptr;
                    }
                }
                if (ImGui::MenuItem("Edit Texture"))
                {
                    const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                    if (!filePath.empty())
                    {
                        m_texture = m_ctx.assetRegistry.Load<Texture>(filePath);
                        m_showTextureWindow = m_texture != nullptr;
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("New Material"))
                {
                    const std::string filePath = SaveFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                    if (!filePath.empty())
                    {
                        m_material = std::make_shared<Material>();
                        m_ctx.assetRegistry.AddAsset<Material>(m_material, filePath, "");
                        if (m_material->IsValid())
                        {
                            m_showMaterialEditor = true;
                            m_ctx.assetRegistry.Save();
                        }
                        else
                        {
                            LOG_WARNING("Could not create material %s", filePath.c_str());
                            m_material = nullptr;
                            m_showMaterialEditor = false;
                        }
                    }
                }
                if (m_ctx.level)
                {
                    ImGui::Separator();
                    if (ImGui::MenuItem("Instantiate Model"))
                    {
                        const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);

                        if (!filePath.empty())
                        {
                            std::shared_ptr<Model> model = m_ctx.assetRegistry.Load<Model>(filePath);
                            // todo: check if model is already in level

                            if (model)
                            {
                                m_ctx.level->models.emplace_back(model);

                                for (size_t i = 0; i < model->meshes.size(); i++)
                                {
                                    auto& instance = m_ctx.level->meshes.emplace_back();
                                    instance.name = model->meshes[i].name;
                                    instance.model = m_ctx.level->models.size() - 1;
                                    instance.mesh = static_cast<uint32_t>(i);
                                    instance.bounds = model->meshes[i].bounds;
                                    instance.modelMatrix = instance.transform.AsMatrix();
                                    const glm::vec3 lightReferencePosition = (instance.bounds.min + instance.bounds.max) / 2.0f;
                                    GetClosestLights(*m_ctx.level, lightReferencePosition, instance.lights);
                                }

                                m_ctx.renderer->InitializeLevel(*m_ctx.level);
                            }
                            else
                            {
                                LOG_ERROR("Could not load model %s", filePath.c_str());
                            }
                        }
                    }
                    if (ImGui::MenuItem("Create Cube"))
                    {
                        auto cubeModel = m_ctx.assetRegistry.Load<Model>("Cube");
                        m_ctx.level->models.emplace_back(cubeModel);

                        auto& obj = m_ctx.level->meshes.emplace_back();
                        obj.name = "Cube";
                        obj.model = m_ctx.level->models.size() - 1;
                        obj.mesh = 0;
                        obj.bounds = cubeModel->meshes[0].bounds;
                        obj.modelMatrix = obj.transform.AsMatrix();
                        m_ctx.renderer->InitializeLevel(*m_ctx.level);
                    }
                }
                if (ImGui::MenuItem("Asset Registry"))
                {
                    m_showRegistryWindow = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Lara Settings"))
            {
                m_showLaraWindow = true;
            }

            if (ImGui::MenuItem("Level Settings"))
            {
                m_showLevelWindow = true;
            }
        }

        ImGui::EndMainMenuBar();

        // Items from menu bar

        if (m_showAnimEditor)
        {
            DrawAnimEditor();
        }

        if (m_showMaterialEditor)
        {
            DrawMaterialEditor();
        }

        if (m_showImportWindow)
        {
            DrawImportWindow();
        }

        if (m_showLaraWindow)
        {
            DrawLaraWindow();
        }

        if (m_showRegistryWindow)
        {
            DrawRegistryWindow();
        }

        if (m_showModelWindow)
        {
            DrawModelWindow();
        }

        if (m_showLevelWindow)
        {
            DrawLevelWindow();
        }

        if (m_showTextureWindow)
        {
            DrawTextureWindow();
        }

        // Inspector

        DrawInspector();

        // Debug Messages

        const ImGuiWindowFlags statsFlags =
            ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize;

        ImGui::SetNextWindowSize({ 350.0f, m_ctx.windowHeight - ImGui::GetTextLineHeightWithSpacing() - 30.0f }, ImGuiCond_Always);
        ImGui::SetNextWindowPos({ 0.0f, ImGui::GetTextLineHeightWithSpacing() }, ImGuiCond_Always);
        ImGui::Begin("Log", 0, statsFlags);
        if (ImGui::Button("Clear"))
        {
            DEBUG_CLEAR();
        }
        Debug::MessageLoop(PrintDebugMessage);
        ImGui::End();

        // Stats

        const Transform& camTransform = m_ctx.camera.transform;
        const glm::vec3 camEulers = m_ctx.camera.transform.EulerRotation();

        ImGui::SetNextWindowSize({ static_cast<float>(m_ctx.windowWidth), 30.0f }, ImGuiCond_Always);
        ImGui::SetNextWindowPos({ 0.0f, m_ctx.windowHeight - 30.0f }, ImGuiCond_Always);
        ImGui::Begin("Stats", 0, statsFlags);
        ImGui::Text("FPS: %i / Camera Position: (%f, %f, %f) / Camera Rotation: (%f, %f, %f)",
            m_ctx.debugData.fps,
            camTransform.position.x,
            camTransform.position.y,
            camTransform.position.z,
            glm::degrees(camEulers.x),
            glm::degrees(camEulers.y),
            glm::degrees(camEulers.z));
        ImGui::End();
    }

    void Editor::DrawMaterialEditor()
    {
        ImGui::SetNextWindowSizeConstraints({ 400, 200 }, { 600, 800 });
        ImGui::Begin("Material Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        if (!m_material)
        {
            ImGui::Text("No material opened");
        }
        else
        {
            ImGui::Text("Name: %s", m_material->name.c_str());
            if (m_material->diffuse)
            {
                ImGui::Text("Diffuse: %s", m_material->diffuse->name.c_str());
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
                    m_material->diffuse = m_ctx.assetRegistry.Load<Texture>(texturePath);
                    if (m_material->diffuse)
                    {
                        m_material->AddFlag(MATERIAL_FLAG_DIFFUSE);
                    }
                    else
                    {
                        m_material->RemoveFlag(MATERIAL_FLAG_DIFFUSE);
                    }
                }
            }
            if (m_material->normal)
            {
                ImGui::Text("Normal: %s", m_material->normal->name.c_str());
            }
            else
            {
                ImGui::Text("Normal: Null");
            }
            if (ImGui::Button("Set Normal"))
            {
                const std::string texturePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                if (!texturePath.empty())
                {
                    m_material->normal = m_ctx.assetRegistry.Load<Texture>(texturePath);
                    if (m_material->normal)
                    {
                        m_material->AddFlag(MATERIAL_FLAG_NORMAL);
                    }
                    else
                    {
                        m_material->RemoveFlag(MATERIAL_FLAG_NORMAL);
                    }
                }
            }
            bool isTransparent = m_material->TestFlag(MATERIAL_FLAG_TRANSPARENT);
            if (ImGui::Checkbox("Is Transparent", &isTransparent))
            {
                if (isTransparent)
                {
                    m_material->AddFlag(MATERIAL_FLAG_TRANSPARENT);
                }
                else
                {
                    m_material->RemoveFlag(MATERIAL_FLAG_TRANSPARENT);
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                m_ctx.assetRegistry.SaveAsset(m_material);
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Close"))
        {
            m_material = nullptr;
            m_showMaterialEditor = false;
        }
        ImGui::End();
    }

    void Editor::DrawAnimEditor()
    {
        ImGui::SetNextWindowSizeConstraints({ 400, 300 }, { 600, 800 });
        ImGui::SetNextWindowSize({ 480, 520 }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Anim Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (!m_animation)
        {
            ImGui::Text("No anim selected");
            ImGui::End();
            return;
        }

        auto& anim = m_animation;

        std::string nameOnly = anim->GetFileName();
        if (ImGui::InputText("Name", &nameOnly))
        {
            anim->SetFileName(nameOnly);
            anim->isDirty = true;
        }

        ImGui::SeparatorText("Settings");
        if (ImGui::Checkbox("Root Motion", &m_animation->hasRootMotion))
        {
            anim->isDirty = true;
        }

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
                    {
                        key.value = (AnimEvent)eventType;
                        anim->isDirty = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::InputFloat("Time", &key.time, 0.0f, 0.0f, "%.2f"))
            {
                if (key.time < 0.0f)
                {
                    key.time = 0.0f;
                }
                else if (key.time > anim->length)
                {
                    key.time = anim->length;
                }
                anim->isDirty = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                anim->events.erase(anim->events.begin() + i);
                ImGui::PopID();
                anim->isDirty = true;
                break; // to avoid issues with changed indices
            }
            ImGui::PopID();
        }

        if (ImGui::Button("Add Event"))
        {
            anim->events.emplace_back();
            anim->isDirty = true;
        }

        ImGui::Separator();

        if (ImGui::Button("Save"))
        {
            std::sort(anim->events.begin(), anim->events.end(), [](EventKey& key1, EventKey& key2) { return key1.time < key2.time; });
            m_ctx.assetRegistry.SaveAsset(anim);
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            m_animation = nullptr;
            m_showAnimEditor = false;
        }

        ImGui::End();
    }

    void Editor::DrawInspector()
    {
        const ImGuiWindowFlags inspectorFlags =
            ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoTitleBar;

        ImGui::SetNextWindowSize({ 350.0f, m_ctx.windowHeight - ImGui::GetTextLineHeightWithSpacing() - 30.0f }, ImGuiCond_Always);
        ImGui::SetNextWindowPos({ m_ctx.windowWidth - 350.0f, ImGui::GetTextLineHeightWithSpacing() }, ImGuiCond_Always);
        ImGui::Begin("Inspector", 0, inspectorFlags);
        ImGui::BeginTabBar("Tabs");
        if (ImGui::BeginTabItem("Inspector"))
        {
            if (!m_ctx.level)
            {
                ImGui::Text("No level loaded");
            }
            else
            {
                static glm::vec3 s_eulerRotation{};
                if (ImGui::BeginListBox("Mesh Instances"))
                {
                    for (size_t i = 0; i < m_ctx.level->meshes.size(); i++)
                    {
                        ImGui::PushID(i);
                        auto& staticObj = m_ctx.level->meshes[i];
                        bool selected = i == m_selectedObject;
                        auto& mesh = m_ctx.level->models[staticObj.model]->meshes[staticObj.mesh];
                        const char* name = staticObj.name.size() > 0 ? staticObj.name.c_str() : "#UNNAMED!";
                        if (ImGui::Selectable(name, &selected))
                        {
                            if (m_selectedObject == i)
                            {
                                s_eulerRotation = {};
                                m_selectedObject = std::numeric_limits<size_t>::max();
                            }
                            else
                            {
                                m_selectedObject = i;
                                s_eulerRotation = staticObj.transform.EulerRotation();
                            }
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndListBox();
                }
                ImGui::SeparatorText("Details");
                if (m_selectedObject < m_ctx.level->meshes.size())
                {
                    auto& obj = m_ctx.level->meshes[m_selectedObject];
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
                        OnObjectTransformUpdate(m_selectedObject);
                    }
                    ImGui::SeparatorText("Mesh Asset");
                    auto& mesh = m_ctx.level->models[obj.model]->meshes[obj.mesh];
                    ImGui::Text("Mesh: %s", mesh.name.c_str());
                    ImGui::Text("Material Name: %s", obj.overrideMaterial ? obj.overrideMaterial->name.c_str() : mesh.material->name.c_str());
                    if (ImGui::Button("Set Material"))
                    {
                        const std::string materialPath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                        if (!materialPath.empty())
                        {
                            if (auto material = m_ctx.assetRegistry.Load<Material>(materialPath); material)
                            {
                                obj.overrideMaterial = material;
                                m_ctx.renderer->InitializeLevel(*m_ctx.level);
                            }
                        }
                    }
                    if (obj.overrideMaterial && ImGui::Button("Clear Override"))
                    {
                        obj.overrideMaterial = nullptr;
                        m_ctx.renderer->InitializeLevel(*m_ctx.level);
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
                        DeleteLevelObject(m_ctx, m_selectedObject);
                    }
                }
            }
            ImGui::EndTabItem();
        }
        if (m_ctx.level && ImGui::BeginTabItem("Lighting"))
        {
            if (ImGui::BeginListBox("Point Lights"))
            {
                for (size_t l = 0; l < m_ctx.level->pointLights.size(); l++)
                {
                    if (m_selectedObject < m_ctx.level->meshes.size())
                    {
                        bool found = false;
                        for (auto& light : m_ctx.level->meshes[m_selectedObject].lights)
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

                    bool selected = m_selectedPointLight == l;
                    if (ImGui::Selectable(std::to_string(l).c_str(), &selected))
                    {
                        m_selectedPointLight = l;
                    }
                }
                ImGui::EndListBox();
            }

            if (ImGui::Button("Add Light"))
            {
                PointLight& light = m_ctx.level->pointLights.emplace_back();
                light.color = glm::vec3(1.0f, 1.0f, 1.0f);
                light.intensity = 1.0f;
                light.innerRadius = 1.0f;
                light.outerRadius = 5.0f;
            }

            if (m_selectedPointLight < m_ctx.level->pointLights.size())
            {
                bool modified = false;
                auto& light = m_ctx.level->pointLights[m_selectedPointLight];
                modified |= ImGui::InputFloat3("Position", &light.position.x);
                modified |= ImGui::InputFloat3("Color", &light.color.r);
                modified |= ImGui::InputFloat("Intensity", &light.intensity);
                modified |= ImGui::InputFloat("Inner Radius", &light.innerRadius);
                modified |= ImGui::InputFloat("Outer Radius", &light.outerRadius);
                if (modified)
                {
                    UpdateAllClosestLights(*m_ctx.level);
                    m_ctx.renderer->UpdateLights(*m_ctx.level);
                }
            }

            ImGui::SeparatorText("Phong Settings");
            ImGui::InputFloat3("Ambient Color", &m_ctx.level->ambientColor.x);
            ImGui::InputFloat("Ambient Strength", &m_ctx.level->ambientStrength);
            ImGui::InputFloat3("Directional Color", &m_ctx.level->directionalLight.color.x);
            ImGui::InputFloat3("Directional Forward", &m_ctx.level->directionalLight.dir.x);
            ImGui::InputFloat("Directional Intensity", &m_ctx.level->directionalLight.intensity);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::End();
    }

    void Editor::DrawImportWindow()
    {
        static ImportSettings settings{};

        ImGui::Begin("Importer", nullptr, ImGuiWindowFlags_NoCollapse 
            | ImGuiWindowFlags_AlwaysAutoResize);

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
                const AssetId id = m_ctx.assetRegistry.GetAssetId(FileIO::GetRelativePath(skeletonPath, m_project.directory));
                if (IsValidAssetId(id))
                {
                    settings.existingSkeleton = m_ctx.assetRegistry.Load<Skeleton>(id);
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

                if (basePath[basePath.size() - 1] == '/')
                {
                    basePath[basePath.size() - 1] = FileIO::Separator;
                }
                else if (basePath[basePath.size() - 1] != '\\')
                {
                    basePath.push_back(FileIO::Separator);
                }

                for (auto& filePath : m_importPaths)
                {
                    m_modelImporter.Start(filePath);

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

                    if (m_modelImporter.Start(filePath))
                    {
                        auto result = m_modelImporter.Import(settings);

                        // Model references skeleton so keep this first
                        if (result.skeleton && settings.importSkeleton)
                        {
                            m_ctx.assetRegistry.AddAsset(result.skeleton, settings.skeletonPath, filePath);
                        }

                        if (result.model && settings.importModel)
                        {
                            for (auto& mesh : result.model->meshes)
                            {
                                if (!mesh.material)
                                {
                                    continue;
                                }

                                // Save material before textures so they exist when material is loaded
                                if (mesh.material->diffuse)
                                {
                                    m_ctx.assetRegistry.AddAsset(mesh.material->diffuse, mesh.material->diffuse->name, filePath);
                                }

                                if (mesh.material->normal)
                                {
                                    m_ctx.assetRegistry.AddAsset(mesh.material->normal, mesh.material->normal->name, filePath);
                                }

                                m_ctx.assetRegistry.AddAsset(mesh.material, mesh.material->name, filePath);
                            }

                            m_ctx.assetRegistry.AddAsset(result.model, settings.modelPath, filePath);
                        }

                        if (result.animation && settings.importAnimation)
                        {
                            m_ctx.assetRegistry.AddAsset(result.animation, settings.animationPath, filePath);
                        }
                    }
                    else
                    {
                        LOG_ERROR("Failed to start import process for %s", filePath.c_str());
                    }
                }

                LOG("Imported Finished");
                m_modelImporter.Finish();
                m_showImportWindow = false;

                m_ctx.assetRegistry.Save();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            m_showImportWindow = false;
        }

        ImGui::End();
    }

    void Editor::DrawLaraWindow()
    {
        ImGui::SetNextWindowSizeConstraints({ 300, 300 }, { 600, 800 });
        ImGui::Begin("Lara Settings", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::InputFloat3("Position", &m_ctx.lara.transform.position.x))
        {
            OnLaraTransformUpdate();
        }

        glm::vec3 eulers = m_ctx.lara.transform.EulerRotation();
        if (ImGui::InputFloat3("Rotation", &eulers.x))
        {
            m_ctx.lara.transform.SetEulers(glm::radians(eulers.x), glm::radians(eulers.y), glm::radians(eulers.z));
            OnLaraTransformUpdate();
        }

        ImGui::SeparatorText("Model");
        ImGui::Text("Asset: %s", m_ctx.lara.model ? m_ctx.lara.model->name.c_str() : "Empty");

        if (ImGui::Button("Set Lara Model"))
        {
            const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
            if (!filePath.empty())
            {
                m_project.laraPath = m_ctx.assetRegistry.GetAssetId(filePath);
                SetLaraModel(m_ctx, m_project.laraPath);
            }
        }

        if (m_ctx.lara.model)
        {
            // todo: pull out into separate window
            ImGui::SeparatorText("Skeleton");
            const auto& skel = m_ctx.lara.model->skeleton;
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

        if (m_ctx.lara.model && ImGui::Button("Save Model"))
        {
            m_ctx.assetRegistry.SaveAsset(m_ctx.lara.model);
            ImGui::SameLine();
        }

        if (ImGui::Button("Close"))
        {
            m_showLaraWindow = false;
        }
        ImGui::End();
    }

    void Editor::DrawModelWindow()
    {
        ImGui::SetNextWindowSize({ 520.0f, 700.0f }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Model Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::SeparatorText("Meshes");
        ImGui::BeginChild("MeshList", { 500, 300 });
        for (size_t i = 0; i < m_model->meshes.size(); i++)
        {
            auto& mesh = m_model->meshes[i];
            ImGui::PushID(i);
            ImGui::Text("Mesh %i: %s", i, mesh.name.c_str());
            ImGui::Text("Material: %s", mesh.material ? mesh.material->name.c_str() : "Empty");

            if (ImGui::Button("Set Material"))
            {
                const std::string& filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
                if (!filePath.empty())
                {
                    mesh.material = m_ctx.assetRegistry.Load<Material>(filePath);
                    m_model->isDirty = true;
                }
            }
            ImGui::Checkbox("Is Active", &mesh.isActive);
            if (ImGui::Button("Delete"))
            {
                m_model->meshes.erase(m_model->meshes.begin() + i);
                m_model->isDirty = true;
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
            ImGui::Separator();
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void Editor::DrawRegistryWindow()
    {
        ImGui::SetNextWindowSize({ 700.0f, 500.0f }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Asset Registry");

        ImGui::Text("Total Assets: %zu", m_ctx.assetRegistry.m_assets.size());
        ImGui::Separator();

        ImGui::Columns(4, "AssetColumns");

        ImGui::Text("ID"); 
        ImGui::NextColumn();

        ImGui::Text("Type"); 
        ImGui::NextColumn();

        ImGui::Text("Asset Path");
        ImGui::NextColumn();

        ImGui::Text("Source Path");
        ImGui::NextColumn();

        ImGui::Separator();

        for (const auto& [id, meta] : m_ctx.assetRegistry.m_assets)
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
            case ASSET_TYPE_SOUND:
                typeStr = "Sound";
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
            m_ctx.assetRegistry.Save();
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            m_showRegistryWindow = false;
        }

        ImGui::End();
    }

    void Editor::DrawLevelWindow()
    {
        ImGui::SetNextWindowSizeConstraints({ 300, 300 }, { 600, 800 });
        ImGui::Begin("Level Settings", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (!m_ctx.level)
        {
            ImGui::Text("No level loaded");
            ImGui::End();
            return;
        }

        ImGui::InputText("Name", &m_ctx.level->name);
        if (ImGui::Button("Set as Default Level"))
        {
            m_project.defaultLevelPath = m_ctx.assetRegistry.GetAssetId(m_ctx.level->GetFileName());
        }
        if (IsValidAssetId(m_project.defaultLevelPath))
        {
            ImGui::Text("Current Default: %s", m_ctx.level->name.c_str());
        }
        else
        {
            ImGui::Text("No Default Level Set");
        }

        ImGui::SeparatorText("Audio");
        ImGui::Text("Ambient Sound: %s", m_ctx.level->ambientSound ? m_ctx.level->ambientSound->name.c_str() : "Empty");
        ImGui::InputFloat("Ambient Volume", &m_ctx.level->ambientSoundVolume, 0.01f, 0.1f, "%.2f");
        if (ImGui::Button("Set Ambient Sound"))
        {
            const std::string filePath = OpenFileDialog(TombSlateFileTypes, NumTombSlateFiles);
            if (!filePath.empty())
            {
                m_ctx.level->ambientSound = m_ctx.assetRegistry.Load<Sound>(filePath);
                m_ctx.level->isDirty = true;
            }
        }

        if (ImGui::Button("Save Level"))
        {
            m_ctx.assetRegistry.SaveAsset(m_ctx.level);
        }
        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            m_showLevelWindow = false;
        }
        ImGui::End();
    }

    void Editor::DrawTextureWindow()
    {
        ImGui::SetNextWindowSize({ 400.0f, 400.0f }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Texture Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        if (!m_texture)
        {
            ImGui::Text("No texture opened");
            ImGui::End();
            return;
        }
        ImGui::Text("Name: %s", m_texture->name.c_str());
        ImGui::Text("Dimensions: %ix%i", m_texture->width, m_texture->height);
        ImGui::Checkbox("sRGB", &m_texture->sRGB);
        ImGui::Separator();
        if (ImGui::Button("Save"))
        {
            m_ctx.assetRegistry.SaveAsset(m_texture);
            ImGui::SameLine();
        }
        if (ImGui::Button("Close"))
        {
            m_texture = nullptr;
            m_showTextureWindow = false;
        }
        ImGui::End();
    }

    void Editor::NewProject(const std::string& path)
    {
        UnloadProject();

        if (!FileIO::IsDirectory(path))
        {
            LOG_ERROR("Could not create project. Path is not a directory: %s", path.c_str());
            return;
        }

        m_project = {};
        m_project.name = FileIO::GetFileName(path);
        m_project.SaveJson(path + "/project.tfproj");

        OnProjectDirectoryUpdate(path);

        m_ctx.level = std::make_shared<Level>();
        m_ctx.level->name = "Untitled Level";

        LOG("Project created: %s at %s", m_project.name.c_str(), path.c_str());
    }

    void Editor::LoadProject(const std::string& settingsPath)
    {
        UnloadProject();

        if (!FileIO::FileExists(settingsPath))
        {
            LOG_ERROR("Project settings file not found: %s", settingsPath.c_str());
            return;
        }

        m_project = {};
        m_project.LoadJson(settingsPath);

        const std::string projectDirectory = FileIO::GetDirectory(settingsPath);
        m_project.directory = projectDirectory;

        OnProjectDirectoryUpdate(projectDirectory);

        if (IsValidAssetId(m_project.laraPath))
        {
            SetLaraModel(m_ctx, m_project.laraPath);
        }

        if (IsValidAssetId(m_project.defaultLevelPath))
        {
            LoadLevel(m_ctx, m_project.defaultLevelPath);
        }
        else
        {
            m_ctx.level = std::make_shared<Level>();
            m_ctx.level->name = "Untitled Level";
        }

        LOG("Project loaded: %s", m_project.name.c_str());
    }

    void Editor::SaveProject()
    {
        if (!FileIO::IsDirectory(m_project.directory))
        {
            LOG_ERROR("Project path is not set, cannot save project settings");
            return;
        }

        m_project.SaveJson(m_project.directory + "/project.tfproj");
        m_ctx.assetRegistry.Save();
    }

    void Editor::UnloadProject()
    {
        UnloadLevel(m_ctx);

        m_project = {};
        m_modelImporter.Finish();
    }

    void Editor::OnProjectDirectoryUpdate(const std::string& directory)
    {
        m_ctx.assetRegistry.Init(directory);
    }

    void Editor::OnObjectTransformUpdate(size_t index)
    {
        if (index >= m_ctx.level->meshes.size())
        {
            LOG_ERROR("Tried to update transform of object index %i but only %i objects in level", index, m_ctx.level->meshes.size());
            return;
        }

        auto& obj = m_ctx.level->meshes[index];
        obj.modelMatrix = obj.transform.AsMatrix();
        UpdateBounds(*m_ctx.level, obj);

        if (m_ctx.level)
        {
            const glm::vec3 lightReferencePosition = (obj.bounds.min + obj.bounds.max) / 2.0f;
            GetClosestLights(*m_ctx.level, lightReferencePosition, obj.lights);
        }
    }

    void Editor::OnLaraTransformUpdate()
    {
        if (!m_ctx.lara.physics)
        {
            LOG_ERROR("Updated Lara's transform but physics is not initialized");
            return;
        }

        m_ctx.lara.physics->SetPosition(GlmVec3ToJph(m_ctx.lara.transform.position));
    }

    void Editor::HandleKey(int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            if (key == GLFW_KEY_F3)
            {
                const bool isEditor = !m_isEditMode;
                m_ctx.isPaused = isEditor;
                m_ctx.isFreeCamera = isEditor;
                m_isEditMode = isEditor;
                if (!m_isEditMode)
                {
                    if (m_ctx.level && m_ctx.level->ambientSound)
                    {
                        m_ctx.audioSystem.PlaySound(m_ctx.level->ambientSound, m_ctx.level->ambientSoundVolume, true);
                    }
                }
                else
                {
                    m_ctx.audioSystem.StopAllSounds();
                }
            }
            else if (key == GLFW_KEY_F4)
            {
                m_showColliders = !m_showColliders;
            }
            else if (key == GLFW_KEY_F5)
            {
                m_showMeshWireframe = !m_showMeshWireframe;
            }
            else if (key == GLFW_KEY_F6)
            {
                m_drawOctree = !m_drawOctree;
            }
            else if (key == GLFW_KEY_F7)
            {
                m_ctx.wantsFrameAdvance = true;
            }
            else if (key == GLFW_KEY_F8)
            {
                m_drawNormals = !m_drawNormals;
            }
        }
    }

    void Editor::HandleMouseButton(int button, int action, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            if (m_isEditMode && !ImGui::GetIO().WantCaptureMouse && m_ctx.level)
            {
                const float mouseX = (2.0f * m_ctx.mouseX) / m_ctx.windowWidth - 1.0f;
                const float mouseY = 1.0f - (2.0f * m_ctx.mouseY) / m_ctx.windowHeight;
                const float mouseZ = -1.0f; // Near plane

                const glm::vec4 ndc{ mouseX, mouseY, mouseZ, 1.0f };
                const glm::mat4 view = glm::inverse(m_ctx.camera.transform.AsMatrix());
                const glm::mat4 projection = glm::perspective(m_ctx.camera.fovY, m_ctx.camera.aspect, m_ctx.camera.near, m_ctx.camera.far);
                const glm::mat4 invVP = glm::inverse(projection * view);

                glm::vec4 worldMouse = invVP * ndc;
                worldMouse /= worldMouse.w;

                const glm::vec3 rayDirection = glm::normalize(glm::vec3(worldMouse) - m_ctx.camera.transform.position);

                float closestIndex = std::numeric_limits<float>::max();
                size_t closestObject = std::numeric_limits<size_t>::max();
                for (size_t i = 0; i < m_ctx.level->meshes.size(); i++)
                {
                    auto& obj = m_ctx.level->meshes[i];
                    m_physicsDebugRenderer->DrawColoredLine(m_ctx.camera.transform.position, rayDirection, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, 30.0f);
                    if (RayIntersectsAABB(obj.bounds, m_ctx.camera.transform.position, rayDirection))
                    {
                        if (IsPointInAABB(obj.bounds, m_ctx.camera.transform.position))
                        {
                            continue; // Don't select if inside the object
                        }

                        float dist = glm::length(obj.transform.position - m_ctx.camera.transform.position);
                        if (dist < closestIndex)
                        {
                            closestIndex = dist;
                            closestObject = i;
                        }
                    }
                }

                if (m_selectedObject == closestObject || closestObject == std::numeric_limits<size_t>::max())
                {
                    m_selectedObject = std::numeric_limits<size_t>::max();
                }
                else
                {
                    m_selectedObject = closestObject;
                }
            }
        }
    }

    void Editor::HandleScroll(float scroll)
    {
        if (m_isEditMode)
        {
            m_ctx.freeCameraSpeed += scroll * ScrollIncreaseRate;
            m_ctx.freeCameraSpeed = Maths::Clamp(m_ctx.freeCameraSpeed, ScrollMinSpeed, ScrollMaxSpeed);
        }
    }
}
