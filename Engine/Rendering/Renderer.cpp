#include "Engine/Rendering/Renderer.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Core/Graphics/Color.h"
#include "Core/IO/FileIO.h"
#include "Engine/Animation/AnimPlayer.h"
#include "Engine/Levels/Level.h"
#include "Engine/Player/Lara.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/ShaderCache.h"

namespace TombForge
{
    namespace
    {
        // Shared UBO data for every frame. Must match the std140 layout in shaders.
        struct PerFrameData
        {
            glm::mat4 view{};
            glm::mat4 projection{};
            glm::vec3 cameraPos{};
            float pad0{}; // Padding to ensure 16 byte alignment
            glm::vec3 ambientColor{};
            float ambientIntensity{};
            glm::vec3 dirLightColor{};
            float dirLightIntensity{};
            glm::vec3 dirLightDirection{};
            float pad1{}; // Padding to ensure 16 byte alignment
        };

        constexpr int NumLinesPerBox = 12;

        constexpr glm::mat4 IdentityMatrix = glm::mat4(1.0f);

        constexpr char const* SkinnedVertexShaderPath = "Shaders\\SkinnedMesh.vert";
        constexpr char const* PBRFragmentShaderPath = "Shaders\\ForwardLit_PBR.frag";
        constexpr char const* StaticVertexShaderPath = "Shaders\\StaticMesh.vert";
        constexpr char const* LineVertexShaderPath = "Shaders\\PositionColor.vert";
        constexpr char const* ColorFragmentShaderPath = "Shaders\\VertexColorUnlit.frag";
        constexpr char const* GizmoFragmentShaderPath = "Shaders\\UniformColorUnlit.frag";

        void AddBoxToLineBuffer(std::vector<Line>& lines, const glm::vec4& color, const glm::vec3& min, const glm::vec3& max)
        {
            const glm::vec3 bottomFrontLeft = min;
            const glm::vec3 bottomFrontRight = { max.x, min.y, min.z };
            const glm::vec3 bottomBackRight = { max.x, min.y, max.z };
            const glm::vec3 bottomBackLeft = { min.x, min.y, max.z };

            const glm::vec3 topFrontLeft = { min.x, max.y, min.z };
            const glm::vec3 topFrontRight = { max.x, max.y, min.z };
            const glm::vec3 topBackRight = max;
            const glm::vec3 topBackLeft = { min.x, max.y, max.z };

            lines.emplace_back(LineVertex{ bottomFrontLeft, color }, LineVertex{ bottomFrontRight, color });
            lines.emplace_back(LineVertex{ bottomFrontRight, color }, LineVertex{ bottomBackRight, color });
            lines.emplace_back(LineVertex{ bottomBackRight, color }, LineVertex{ bottomBackLeft, color });
            lines.emplace_back(LineVertex{ bottomBackLeft, color }, LineVertex{ bottomFrontLeft, color });

            lines.emplace_back(LineVertex{ bottomFrontLeft, color }, LineVertex{ topFrontLeft, color });
            lines.emplace_back(LineVertex{ bottomFrontRight, color }, LineVertex{ topFrontRight, color });
            lines.emplace_back(LineVertex{ bottomBackRight, color }, LineVertex{ topBackRight, color });
            lines.emplace_back(LineVertex{ bottomBackLeft, color }, LineVertex{ topBackLeft, color });

            lines.emplace_back(LineVertex{ topFrontLeft, color }, LineVertex{ topFrontRight, color });
            lines.emplace_back(LineVertex{ topFrontRight, color }, LineVertex{ topBackRight, color });
            lines.emplace_back(LineVertex{ topBackRight, color }, LineVertex{ topBackLeft, color });
            lines.emplace_back(LineVertex{ topBackLeft, color }, LineVertex{ topFrontLeft, color });
        }
    }

    Renderer::Renderer()
        : m_graphics{ Graphics::Get() }
    {
    }

    void Renderer::Initialize(int width, int height)
    {
        m_graphics.Initialize(width, height);

        InitializeShaders();
        InitializeDefaultTextures();

        m_bonesUbo = m_graphics.CreateUbo();
        m_perFrameUbo = m_graphics.CreateUbo();
    }

    void Renderer::Destroy()
    {
        m_graphics.DestroyUbo(m_perFrameUbo);
        m_graphics.DestroyUbo(m_bonesUbo);

        if (m_lightsTexture.gpuHandle.IsValid())
        {
            m_graphics.DestroyTextureInstance(m_lightsTexture.gpuHandle);
        }

        if (m_whiteTexture.gpuHandle.IsValid())
        {
            m_graphics.DestroyTextureInstance(m_whiteTexture.gpuHandle);
        }

        if (m_magentaTexture.gpuHandle.IsValid())
        {
            m_graphics.DestroyTextureInstance(m_magentaTexture.gpuHandle);
        }

        if (m_flatNormal.gpuHandle.IsValid())
        {
            m_graphics.DestroyTextureInstance(m_flatNormal.gpuHandle);
        }

        if (m_singleChannelWhite.gpuHandle.IsValid())
        {
            m_graphics.DestroyTextureInstance(m_singleChannelWhite.gpuHandle);
        }
    }

    bool Renderer::InitializeLevel(Level& level)
    {
        UpdateLights(level);

        for (auto& obj : level.meshes)
        {
            if (obj.model > level.models.size() || obj.mesh > level.models[obj.model]->meshes.size())
            {
                continue;
            }

            auto& mesh = level.models[obj.model]->meshes[obj.mesh];
            if (!mesh.gpuHandle.IsValid())
            {
                mesh.gpuHandle = m_graphics.CreateMeshInstance(mesh);
            }

            if (obj.overrideMaterial)
            {
                InitializeMaterial(*obj.overrideMaterial);
            }
            else if (mesh.material)
            {
                InitializeMaterial(*mesh.material);
            }
        }

        m_octTree.Build(level);

        m_opaqueQueue.clear();
        m_transparentQueue.clear();
        m_opaqueQueue.reserve(level.meshes.size());
        m_transparentQueue.reserve(level.meshes.size());

        return true;
    }

    bool Renderer::InitializeTexture(Texture& texture)
    {
        if (texture.gpuHandle.IsValid())
        {
            return true;
        }
        texture.gpuHandle = m_graphics.CreateTextureInstance(texture);
        return texture.gpuHandle.IsValid();
    }

    void Renderer::DeloadLevel(Level& level)
    {
        m_octTree.nodes.clear();

        for (auto& meshInfo : level.meshes)
        {
            auto& mesh = level.models[meshInfo.model]->meshes[meshInfo.mesh];

            if (mesh.gpuHandle.IsValid())
            {
                m_graphics.DestroyMeshInstance(mesh.gpuHandle);
            }

            if (mesh.material)
            {
                DestroyMaterial(*mesh.material);
            }
        }
    }

    void Renderer::UpdateLights(Level& level)
    {
        SubmitLightsTexture(level.pointLights);
    }

    void Renderer::ClearFramebuffer()
    {
        m_graphics.ClearFrameBuffer();
    }

    void Renderer::RenderLevel(const Level& level, const Lara& lara, const Camera& camera)
    {
        m_graphics.UseShader(m_skinnedShader->GetHandle());

        m_viewMatrix = glm::inverse(camera.transform.AsMatrix());
        m_projectionMatrix = glm::perspective(camera.fovY, camera.aspect, camera.near, camera.far);

        PerFrameData perFrameData{};
        perFrameData.view = m_viewMatrix;
        perFrameData.projection = m_projectionMatrix;
        perFrameData.cameraPos = camera.transform.position;
        perFrameData.ambientColor = SRGBToLinear(level.ambientColor);
        perFrameData.ambientIntensity = level.ambientStrength;
        perFrameData.dirLightColor = SRGBToLinear(level.directionalLight.color);
        perFrameData.dirLightIntensity = level.directionalLight.intensity;
        perFrameData.dirLightDirection = glm::normalize(level.directionalLight.dir);

        m_graphics.UpdateUbo(m_perFrameUbo, &perFrameData, sizeof(PerFrameData));
        m_graphics.BindUbo(m_perFrameUbo, 1); // Binding point 1 for per-frame data

        if (m_lightsTexture.gpuHandle.IsValid())
        {
            m_graphics.SetTexture(m_skinnedLocations.lights, m_lightsTexture.gpuHandle, 4);
        }

        Frustum cameraPlanes{};
        ExtractCameraPlanes(cameraPlanes, m_projectionMatrix * m_viewMatrix);

        m_opaqueQueue.clear();
        m_transparentQueue.clear();
        FillRenderQueues(0, level, cameraPlanes, m_opaqueQueue, m_transparentQueue);
        PerformRenderPass(level, m_opaqueQueue, m_transparentQueue);

        RenderLara(level, lara, camera);
    }

    void Renderer::RenderLara(const Level& level, const Lara& lara, const Camera& camera)
    {
        if (lara.model)
        {
            uint8_t lightCount{};
            MeshLightArray lightIndices{};
            GetClosestLights(level, lara.transform.position, lightIndices, lightCount);

            const auto& finalMatrices = lara.animPlayer.FinalBoneMatrices();

            Transform finalTransform = lara.transform;
            finalTransform.rotation *= glm::quat(lara.modelRotationOffset);
            DrawModel(*lara.model.get(), finalTransform, lightIndices, lightCount, false, &finalMatrices);
            DrawModel(*lara.model.get(), finalTransform, lightIndices, lightCount, true, &finalMatrices);
        }
    }

    void Renderer::SetCamera(const Transform& transform, float fovY, float aspect, float near, float far)
    {
        m_viewMatrix = glm::inverse(transform.AsMatrix());
        m_graphics.SetMatrix4("view", m_viewMatrix);

        m_projectionMatrix = glm::perspective(fovY, aspect, near, far);
        m_graphics.SetMatrix4("projection", m_projectionMatrix);
    }

    void Renderer::OnWindowResized(int width, int height)
    {
        m_graphics.ResizeFramebuffer(width, height);
    }

    bool Renderer::InitializeModel(Model& model)
    {
        for (auto& mesh : model.meshes)
        {
            if (!mesh.gpuHandle.IsValid())
            {
                mesh.gpuHandle = m_graphics.CreateMeshInstance(mesh);
            }

            if (mesh.material)
            {
                InitializeMaterial(*mesh.material);
            }
        }

        return true;
    }

    void Renderer::RenderWireframe(const Model& model, const Transform& transform, const Camera& camera)
    {
        m_graphics.UseShader(m_gizmoShader->GetHandle()); // Just does a color - same thing
        m_graphics.SetVec4("color", { 1.0f, 1.0f, 1.0f, 1.0f });
        m_graphics.SetMatrix4("model", transform.AsMatrix());

        SetCamera(camera.transform, camera.fovY, camera.aspect, camera.near, camera.far);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        for (auto& mesh : model.meshes)
        {
            m_graphics.DrawMesh(mesh.gpuHandle);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void Renderer::RenderWireframe(const Mesh& model, const Transform& transform, const Camera& camera)
    {
        m_graphics.UseShader(m_gizmoShader->GetHandle()); // Just does a color - same thing
        m_graphics.SetVec4("color", { 1.0f, 1.0f, 1.0f, 1.0f });
        m_graphics.SetMatrix4("model", transform.AsMatrix());

        SetCamera(camera.transform, camera.fovY, camera.aspect, camera.near, camera.far);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        m_graphics.DrawMesh(model.gpuHandle);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void Renderer::DrawOctree(const glm::vec4& color, const Camera& camera, uint32_t contains)
    {
        std::vector<Line> lines{};
        lines.reserve(m_octTree.nodes.size() * NumLinesPerBox);

        for (const auto& node : m_octTree.nodes)
        {
            // Means we want to draw the nodes that contain index "contains"
            if (contains != UINT32_MAX)
            {
                bool found = false;
                for (uint32_t c : node.contains)
                {
                    if (c == contains)
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
            AddBoxToLineBuffer(lines, color, node.bounds.min, node.bounds.max);
        }

        m_graphics.UseShader(m_lineShader->GetHandle());

        SetCamera(camera.transform, camera.fovY, camera.aspect, camera.near, camera.far);

        m_graphics.SetMatrix4("model", glm::mat4{ 1.0f });
        m_graphics.DrawLines(lines);
    }

    void Renderer::DrawBox(const AABB& aabb, const glm::vec4& color, const Camera& camera)
    {
        std::vector<Line> lines{};
        lines.reserve(NumLinesPerBox);
        AddBoxToLineBuffer(lines, color, aabb.min, aabb.max);

        m_graphics.UseShader(m_lineShader->GetHandle());

        SetCamera(camera.transform, camera.fovY, camera.aspect, camera.near, camera.far);

        m_graphics.SetMatrix4("model", glm::mat4{ 1.0f });
        m_graphics.DrawLines(lines);
    }

    void Renderer::InitializeShaders()
    {
        ShaderCache::Get().Initialize();
        m_baseShader = ShaderCache::Get().GetStaticMeshShader();
        m_skinnedShader = ShaderCache::Get().GetSkinnedMeshShader();
        m_depthShader = ShaderCache::Get().GetDepthShader();
        m_lineShader = ShaderCache::Get().GetLineShader();
        m_gizmoShader = ShaderCache::Get().GetGizmoShader();
        m_graphics.UseShader(m_skinnedShader->GetHandle());

        m_skinnedShader->CacheLocations({
            "diffuseTexture",
            "normalTexture",
            "roughnessTexture",
            "metalnessTexture",
            "material.albedoColor",
            "material.roughnessValue",
            "material.metalnessValue",
            "roughnessChannel",
            "metalnessChannel",
            "lightsTexture",
            "numLights",
            "lightIndices[0]",
            "lightIndices[1]",
            "lightIndices[2]",
            "lightIndices[3]",
            "lightIndices[4]",
            "lightIndices[5]",
            "lightIndices[6]",
            "lightIndices[7]",
            "model"
            });

        m_skinnedLocations.albedoTexture = m_skinnedShader->GetLocation("diffuseTexture");
        m_skinnedLocations.normalTexture = m_skinnedShader->GetLocation("normalTexture");
        m_skinnedLocations.roughnessTexture = m_skinnedShader->GetLocation("roughnessTexture");
        m_skinnedLocations.metalnessTexture = m_skinnedShader->GetLocation("metalnessTexture");

        m_skinnedLocations.albedoColor = m_skinnedShader->GetLocation("material.albedoColor");
        m_skinnedLocations.roughnessValue = m_skinnedShader->GetLocation("material.roughnessValue");
        m_skinnedLocations.metalnessValue = m_skinnedShader->GetLocation("material.metalnessValue");
        m_skinnedLocations.roughnessChannel = m_skinnedShader->GetLocation("roughnessChannel");
        m_skinnedLocations.metalnessChannel = m_skinnedShader->GetLocation("metalnessChannel");

        m_skinnedLocations.lights = m_skinnedShader->GetLocation("lightsTexture");
        m_skinnedLocations.numLights = m_skinnedShader->GetLocation("numLights");
        m_skinnedLocations.lightIndices[0] = m_skinnedShader->GetLocation("lightIndices[0]");
        m_skinnedLocations.lightIndices[1] = m_skinnedShader->GetLocation("lightIndices[1]");
        m_skinnedLocations.lightIndices[2] = m_skinnedShader->GetLocation("lightIndices[2]");
        m_skinnedLocations.lightIndices[3] = m_skinnedShader->GetLocation("lightIndices[3]");
        m_skinnedLocations.lightIndices[4] = m_skinnedShader->GetLocation("lightIndices[4]");
        m_skinnedLocations.lightIndices[5] = m_skinnedShader->GetLocation("lightIndices[5]");
        m_skinnedLocations.lightIndices[6] = m_skinnedShader->GetLocation("lightIndices[6]");
        m_skinnedLocations.lightIndices[7] = m_skinnedShader->GetLocation("lightIndices[7]");

        m_skinnedLocations.modelMatrix = m_skinnedShader->GetLocation("model");
    }

    void Renderer::InitializeDefaultTextures()
    {
        // Setup white texture
        constexpr ColorByte white = 255;
        m_whiteTexture.width = m_whiteTexture.height = 1;
        m_whiteTexture.format = TextureFormat::RGB;
        m_whiteTexture.data.emplace_back(white);
        m_whiteTexture.data.emplace_back(white);
        m_whiteTexture.data.emplace_back(white);
        m_whiteTexture.sRGB = true;
        m_whiteTexture.gpuHandle = m_graphics.CreateTextureInstance(m_whiteTexture);

        // Setup (error) magenta texture
        m_magentaTexture.width = m_magentaTexture.height = 1;
        m_magentaTexture.format = TextureFormat::RGB;
        m_magentaTexture.data.emplace_back(white);
        m_magentaTexture.data.emplace_back(0);
        m_magentaTexture.data.emplace_back(white);
        m_magentaTexture.sRGB = true;
        m_magentaTexture.gpuHandle = m_graphics.CreateTextureInstance(m_magentaTexture);

        // Setup flat normal map
        constexpr ColorByte normalRG = 128;
        m_flatNormal.width = m_flatNormal.height = 1;
        m_flatNormal.format = TextureFormat::RGB;
        m_flatNormal.data.emplace_back(normalRG);
        m_flatNormal.data.emplace_back(normalRG);
        m_flatNormal.data.emplace_back(white);
        m_flatNormal.sRGB = false;
        m_flatNormal.gpuHandle = m_graphics.CreateTextureInstance(m_flatNormal);

        // Setup white single channel texture (for roughness and metallic)
        m_singleChannelWhite.width = m_singleChannelWhite.height = 1;
        m_singleChannelWhite.format = TextureFormat::R;
        m_singleChannelWhite.data.emplace_back(white);
        m_singleChannelWhite.sRGB = false;
        m_singleChannelWhite.filter = TextureFilter::Nearest;
        m_singleChannelWhite.gpuHandle = m_graphics.CreateTextureInstance(m_singleChannelWhite);
    }

    void Renderer::SubmitLightsTexture(const std::vector<PointLight>& lights)
    {
        if (lights.size() < 1)
        {
            // todo: default this to a black placeholder texture
            return;
        }

        const size_t textureSize = lights.size() * sizeof(PointLight);

        m_lightsTexture.width = lights.size() * 9; // 9 floats make up a point light
        m_lightsTexture.height = 1;
        m_lightsTexture.format = TextureFormat::R;
        m_lightsTexture.type = TextureDataType::Float;
        m_lightsTexture.filter = TextureFilter::Nearest;
        m_lightsTexture.data.resize(textureSize);

        // All the lights should already be in linear space, so no conversion here
        memcpy(m_lightsTexture.data.data(), lights.data(), textureSize);

        m_lightsTexture.gpuHandle = m_graphics.CreateTextureInstance(m_lightsTexture);
    }

    void Renderer::FillRenderQueues(uint32_t nodeIndex,
        const Level& level,
        const Frustum& frustum,
        std::vector<uint32_t>& opaque,
        std::vector<uint32_t>& transparent)
    {
        if (nodeIndex >= m_octTree.nodes.size())
        {
            return;
        }

        const auto& node = m_octTree.nodes[nodeIndex];

        if (FrustumIntersectsAABB(frustum, node.bounds))
        {
            for (uint32_t item : node.contains)
            {
                const MeshInstance& obj = level.meshes[item];
                auto& mesh = level.models[obj.model]->meshes[obj.mesh];

                if (mesh.material && mesh.material->TestFlag(MATERIAL_FLAG_TRANSPARENT))
                {
                    transparent.emplace_back(item);
                }
                else
                {
                    opaque.emplace_back(item);
                }
            }

            for (uint32_t subIndex : node.subNodeIndices)
            {
                if (subIndex == 0)
                {
                    continue;
                }

                FillRenderQueues(subIndex, level, frustum, opaque, transparent);
            }
        }
    }

    void Renderer::PerformRenderPass(const Level& level, std::vector<uint32_t>& opaque, std::vector<uint32_t>& transparent)
    {
        m_graphics.UpdateUbo(m_bonesUbo, m_boneMatrixBuffer.data(), m_boneMatrixBuffer.size() * sizeof(glm::mat4));
        m_graphics.BindUbo(m_bonesUbo, 0);

        for (uint32_t objIndex : opaque)
        {
            auto& meshInfo = level.meshes[objIndex];
            auto& mesh = level.models[meshInfo.model]->meshes[meshInfo.mesh];
            m_graphics.SetMatrix4(m_skinnedLocations.modelMatrix, level.meshes[objIndex].modelMatrix);
            DrawMesh(mesh, meshInfo.lights, meshInfo.lightCount, meshInfo.overrideMaterial ? meshInfo.overrideMaterial.get() : nullptr);
        }

        for (uint32_t objIndex : transparent)
        {
            auto& meshInfo = level.meshes[objIndex];
            auto& mesh = level.models[meshInfo.model]->meshes[meshInfo.mesh];
            m_graphics.SetMatrix4(m_skinnedLocations.modelMatrix, level.meshes[objIndex].modelMatrix);
            DrawMesh(mesh, meshInfo.lights, meshInfo.lightCount, meshInfo.overrideMaterial ? meshInfo.overrideMaterial.get() : nullptr);
        }
    }

    void Renderer::SetMaterial(const Material& material)
    {
        if (material.albedoTexture)
        {
            // This is checked inside if so we can have a magenta fallback for missing textures
            const bool isValid = material.albedoTexture->gpuHandle.IsValid();
            m_graphics.SetTexture(m_skinnedLocations.albedoTexture, isValid ? material.albedoTexture->gpuHandle : m_magentaTexture.gpuHandle, 0);
        }
        else
        {
            m_graphics.SetTexture(m_skinnedLocations.albedoTexture, m_whiteTexture.gpuHandle, 0);
        }
        m_graphics.SetVec4(m_skinnedLocations.albedoColor, material.albedoColor);

        if (TextureValidOnGPU(material.normalTexture.get()))
        {
            m_graphics.SetTexture(m_skinnedLocations.normalTexture, material.normalTexture->gpuHandle, 1);
        }
        else
        {
            m_graphics.SetTexture(m_skinnedLocations.normalTexture, m_flatNormal.gpuHandle, 1);
        }

        if (TextureValidOnGPU(material.roughnessTexture.get()))
        {
            m_graphics.SetTexture(m_skinnedLocations.roughnessTexture, material.roughnessTexture->gpuHandle, 2);
            m_graphics.SetFloat(m_skinnedLocations.roughnessValue, 1.0f); // Ignore roughness value if using texture
        }
        else
        {
            m_graphics.SetTexture(m_skinnedLocations.roughnessTexture, m_singleChannelWhite.gpuHandle, 2);
            m_graphics.SetFloat(m_skinnedLocations.roughnessValue, material.roughnessValue);
        }
        m_graphics.SetInt(m_skinnedLocations.roughnessChannel, static_cast<int>(material.roughnessChannel));

        if (TextureValidOnGPU(material.metalnessTexture.get()))
        {
            m_graphics.SetTexture(m_skinnedLocations.metalnessTexture, material.metalnessTexture->gpuHandle, 3);
            m_graphics.SetFloat(m_skinnedLocations.metalnessValue, 1.0f); // Ignore metallic value if using texture
        }
        else
        {
            m_graphics.SetTexture(m_skinnedLocations.metalnessTexture, m_singleChannelWhite.gpuHandle, 3);
            m_graphics.SetFloat(m_skinnedLocations.metalnessValue, material.metalnessValue);
        }
        m_graphics.SetInt(m_skinnedLocations.metalnessChannel, static_cast<int>(material.metalnessChannel));

        m_graphics.BindUbo(m_bonesUbo, 0);
    }

    void Renderer::InitializeMaterial(Material& material)
    {
        if (ValidTextureNotOnGPU(material.albedoTexture.get()))
        {
            material.albedoTexture->gpuHandle = m_graphics.CreateTextureInstance(*material.albedoTexture);
        }
        if (ValidTextureNotOnGPU(material.normalTexture.get()))
        {
            material.normalTexture->gpuHandle = m_graphics.CreateTextureInstance(*material.normalTexture);
        }
        if (ValidTextureNotOnGPU(material.roughnessTexture.get()))
        {
            material.roughnessTexture->gpuHandle = m_graphics.CreateTextureInstance(*material.roughnessTexture);
        }
        if (ValidTextureNotOnGPU(material.metalnessTexture.get()))
        {
            material.metalnessTexture->gpuHandle = m_graphics.CreateTextureInstance(*material.metalnessTexture);
        }
    }

    void Renderer::DestroyMaterial(Material& material)
    {
        if (TextureValidOnGPU(material.albedoTexture.get()))
        {
            m_graphics.DestroyTextureInstance(material.albedoTexture->gpuHandle);
        }
        if (TextureValidOnGPU(material.normalTexture.get()))
        {
            m_graphics.DestroyTextureInstance(material.normalTexture->gpuHandle);
        }
        if (TextureValidOnGPU(material.roughnessTexture.get()))
        {
            m_graphics.DestroyTextureInstance(material.roughnessTexture->gpuHandle);
        }
        if (TextureValidOnGPU(material.metalnessTexture.get()))
        {
            m_graphics.DestroyTextureInstance(material.metalnessTexture->gpuHandle);
        }
    }

    void Renderer::DrawModel(const Model& model,
        const Transform& transform, 
        const MeshLightArray& lightIndices,
        const uint8_t lightCount,
        bool transparentPass,
        const std::vector<glm::mat4>* boneMatrices)
    {
        if (boneMatrices)
        {
            m_graphics.UpdateUbo(m_bonesUbo, boneMatrices->data(), boneMatrices->size() * sizeof(glm::mat4));
        }

        for (size_t m = 0; m < model.meshes.size(); m++)
        {
            m_graphics.SetMatrix4(m_skinnedLocations.modelMatrix, transform.AsMatrix());
            DrawMesh(model.meshes[m], lightIndices, lightCount, transparentPass);
        }
    }

    void Renderer::DrawModelDepth(const Model& model, const Transform& transform)
    {
        for (size_t m = 0; m < model.meshes.size(); m++)
        {
            auto& mesh = model.meshes[m];

            if (!mesh.isActive || !mesh.gpuHandle.IsValid())
            {
                continue;
            }

            if (mesh.isDoubleSided)
            {
                glDisable(GL_CULL_FACE);
            }

            m_graphics.SetMatrix4("model", transform.AsMatrix());
            m_graphics.DrawMesh(mesh.gpuHandle);

            if (mesh.isDoubleSided)
            {
                glEnable(GL_CULL_FACE);
            }
        }
    }

    void Renderer::DrawMesh(const Mesh& mesh,
        const MeshLightArray& lightIndices,
        const uint8_t lightCount,
        const bool isTransparentPass,
        const Material* overrideMaterial)
    {
        if (!mesh.isActive || !mesh.gpuHandle.IsValid())
        {
            return;
        }

        const auto material = overrideMaterial ? overrideMaterial : mesh.material.get();
        if (material)
        {
            const bool isTransparent = material->TestFlag(MATERIAL_FLAG_TRANSPARENT);
            const bool shouldDraw = (isTransparentPass && isTransparent) || (!isTransparentPass && !isTransparent);
            if (!shouldDraw)
            {
                return;
            }
            SetMaterial(*material);
        }

        for (size_t i = 0; i < lightIndices.size() && i < MaxLightsPerMesh; i++)
        {
            m_graphics.SetInt(m_skinnedLocations.lightIndices[i], lightIndices[i]);
        }
        m_graphics.SetInt(m_skinnedLocations.numLights, lightCount);

        m_graphics.SetFaceCulling(!mesh.isDoubleSided);
        m_graphics.DrawMesh(mesh.gpuHandle);
    }

    void Renderer::ExtractCameraPlanes(Frustum& result, const glm::mat4& viewProj) const
    {
        // Remember GLM is column-major, so col is first [] argument
        result.left.a = viewProj[0][3] + viewProj[0][0];
        result.left.b = viewProj[1][3] + viewProj[1][0];
        result.left.c = viewProj[2][3] + viewProj[2][0];
        result.left.d = viewProj[3][3] + viewProj[3][0];

        result.right.a = viewProj[0][3] - viewProj[0][0];
        result.right.b = viewProj[1][3] - viewProj[1][0];
        result.right.c = viewProj[2][3] - viewProj[2][0];
        result.right.d = viewProj[3][3] - viewProj[3][0];

        result.top.a = viewProj[0][3] - viewProj[0][1];
        result.top.b = viewProj[1][3] - viewProj[1][1];
        result.top.c = viewProj[2][3] - viewProj[2][1];
        result.top.d = viewProj[3][3] - viewProj[3][1];

        result.bottom.a = viewProj[0][3] + viewProj[0][1];
        result.bottom.b = viewProj[1][3] + viewProj[1][1];
        result.bottom.c = viewProj[2][3] + viewProj[2][1];
        result.bottom.d = viewProj[3][3] + viewProj[3][1];

        result.near.a = viewProj[0][3] + viewProj[0][2];
        result.near.b = viewProj[1][3] + viewProj[1][2];
        result.near.c = viewProj[2][3] + viewProj[2][2];
        result.near.d = viewProj[3][3] + viewProj[3][2];

        result.far.a = viewProj[0][3] - viewProj[0][2];
        result.far.b = viewProj[1][3] - viewProj[1][2];
        result.far.c = viewProj[2][3] - viewProj[2][2];
        result.far.d = viewProj[3][3] - viewProj[3][2];

        for (auto& plane : result.planes)
        {
            NormalizePlane(plane);
        }
    }

    bool OctTree::Build(const Level& level, const float minNodeSize)
    {
        nodes.clear();

        OctNode& root = nodes.emplace_back();
        root.bounds = CalculateLevelBounds(level);
        ClampBounds(root.bounds, { -500.0f, -500.0f, -500.0f }, { 500.0f, 500.f, 500.0f });

        bool success{ true };
        for (size_t i = 0; i < level.meshes.size(); i++)
        {
            const MeshInstance& obj = level.meshes[i];
            const uint32_t objIndex = static_cast<uint32_t>(i);
            ASSERT(objIndex == i, "Trying to add too many objects to the rendering octree");
            success &= Insert(objIndex, obj.bounds, minNodeSize);
        }
        return success;
    }

    void CalculateOctBounds(OctNode& newBox, const OctNode& parent, OctDirection direction)
    {
        const glm::vec3 newDimensions = (parent.bounds.max - parent.bounds.min) / 2.0f;
        const glm::vec3 parentCenter = (parent.bounds.max + parent.bounds.min) / 2.0f;

        switch (direction)
        {
        case OCT_TOP_FORWARD_LEFT:
            newBox.bounds.min = parentCenter + glm::vec3{ -newDimensions.x, 0.0f, -newDimensions.z };
            break;
        case OCT_TOP_FORWARD_RIGHT:
            newBox.bounds.min = parentCenter + glm::vec3{ 0.0f, 0.0f, -newDimensions.z };
            break;
        case OCT_TOP_BACK_LEFT:
            newBox.bounds.min = parentCenter + glm::vec3{ -newDimensions.x, 0.0f, 0.0f };
            break;
        case OCT_TOP_BACK_RIGHT:
            newBox.bounds.min = parentCenter;
            break;
        case OCT_BOTTOM_FORWARD_LEFT:
            newBox.bounds.min = parentCenter + glm::vec3{ -newDimensions.x, -newDimensions.y, -newDimensions.z };
            break;
        case OCT_BOTTOM_FORWARD_RIGHT:
            newBox.bounds.min = parentCenter + glm::vec3{ 0.0f, -newDimensions.y, -newDimensions.z };
            break;
        case OCT_BOTTOM_BACK_LEFT:
            newBox.bounds.min = parentCenter + glm::vec3{ -newDimensions.x, -newDimensions.y, 0.0f };
            break;
        case OCT_BOTTOM_BACK_RIGHT:
            newBox.bounds.min = parentCenter + glm::vec3{ 0.0f, -newDimensions.y, 0.0f };
            break;
        }

        newBox.bounds.max = newBox.bounds.min + newDimensions;

        ASSERT(newBox.bounds.max.x > newBox.bounds.min.x, "Invalid Octree bounding box");
        ASSERT(newBox.bounds.max.y > newBox.bounds.min.y, "Invalid Octree bounding box");
        ASSERT(newBox.bounds.max.z > newBox.bounds.min.z, "Invalid Octree bounding box");
    }

    bool OctTree::Insert(uint32_t objIndex, const AABB& aabb, const float minNodeSize, uint32_t nodeIndex, uint8_t depth)
    {
        if (!AABBIntersect(nodes[nodeIndex].bounds, aabb))
        {
            // This object is completely outside this oct node
            return false;
        }

        if (depth > 8 || (nodes[nodeIndex].bounds.max - nodes[nodeIndex].bounds.min).x < minNodeSize * 2.0f)
        {
            // We found a node in which this object intersects with so we add it as we reached a leaf node
            nodes[nodeIndex].contains.emplace_back(objIndex);
            return true;
        }

        // Expand node if it hasn't already
        for (uint8_t dir = 0; dir < 8; dir++)
        {
            if (nodes[nodeIndex].subNodeIndices[dir] != 0)
                break;

            OctNode& newNode = nodes.emplace_back();
            newNode.parent = nodeIndex;

            CalculateOctBounds(newNode, nodes[nodeIndex], (OctDirection)dir);

            nodes[nodeIndex].subNodeIndices[dir] = nodes.size() - 1;
        }

        // Check if this is the smallest BV to fully contain this
        bool placeHere = true;
        for (uint8_t dir = 0; dir < 8; dir++)
        {
            const OctNode& subNode = nodes[nodes[nodeIndex].subNodeIndices[dir]];
            if (AABBIsContained(subNode.bounds, aabb))
            {
                // One of the sub nodes can contain this obj more tightly
                placeHere = false;
                break;
            }
        }

        if (placeHere)
        {
            // Don't want to form unneccessary leaves that take up more computation time
            nodes[nodeIndex].contains.emplace_back(objIndex);
            return true;
        }

        // Recurse if the mesh can fit in a smaller BV
        bool inserted = false;
        for (uint8_t i = 0; i < 8; i++)
        {
            const uint32_t subNodeIndex = nodes[nodeIndex].subNodeIndices[i];
            inserted |= Insert(objIndex, aabb, minNodeSize, subNodeIndex, depth + 1);
        }

        return inserted;
    }
}
