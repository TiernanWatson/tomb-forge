#include "Renderer.h"

#include "../../Core/Graphics/Graphics.h"
#include "../../Core/Graphics/Material.h"
#include "../../Core/Graphics/Texture.h"
#include "../../Core/IO/FileIO.h"
#include "../Animation/AnimPlayer.h"
#include "../Levels/Level.h"
#include "../Player/Lara.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/geometric.hpp>

namespace TombForge
{
    static constexpr char const* SkinnedVertexShaderPath = "Shaders\\SkinnedVertexShader.glsl";

    static constexpr char const* BaseFragmentShaderPath = "Shaders\\BaseFragmentShader.glsl";
    static constexpr char const* BaseVertexShaderPath = "Shaders\\BaseVertexShader.glsl";

    static constexpr char const* LineVertexShaderPath = "Shaders\\LineVertexShader.glsl";

    static constexpr char const* ColorFragmentShaderPath = "Shaders\\ColorFragmentShader.glsl";

    static constexpr char const* GizmoFragmentShaderPath = "Shaders\\GizmoFragmentShader.glsl";

    static constexpr char const* DepthFragmentShaderPath = "Shaders\\DepthFragmentShader.glsl";
    static constexpr char const* DepthVertexShaderPath = "Shaders\\DepthVertexShader.glsl";

    Renderer::Renderer()
        : m_graphics{ Graphics::Get() }
    {
        InitializeShaders();
    }

    bool Renderer::InitializeLevel(Level& level)
    {
        for (auto& obj : level.staticObjects)
        {
            if (!obj.model)
            {
                continue;
            }

            InitializeModel(*obj.model);
        }

        SubmitLightsTexture(level.pointLights);

        for (auto& obj : level.meshes)
        {
            if (!obj.mesh)
            {
                continue;
            }

            auto& mesh = *obj.mesh;

            if (!mesh.gpuHandle.IsValid())
            {
                mesh.gpuHandle = m_graphics.CreateMeshInstance(mesh);
            }

            if (mesh.material && mesh.material->diffuse)
            {
                if (!mesh.material->diffuse->gpuHandle.IsValid())
                {
                    mesh.material->diffuse->gpuHandle = m_graphics.CreateTextureInstance(*mesh.material->diffuse);
                    if (!mesh.material->diffuse->gpuHandle.IsValid())
                    {
                        LOG_ERROR("Could not initialize texture: %s", mesh.material->diffuse->name.c_str());
                    }
                }
            }
        }

        m_octTree.Build(level);

        m_opaqueQueue.clear();
        m_transparentQueue.clear();

        m_opaqueQueue.reserve(level.meshes.size());
        m_transparentQueue.reserve(level.meshes.size());

        return true;
    }

    void Renderer::DeloadLevel(Level& level)
    {
        for (auto& obj : level.staticObjects)
        {
            for (auto& mesh : obj.model->meshes)
            {
                if (mesh.gpuHandle.IsValid())
                {
                    m_graphics.DestroyMeshInstance(mesh.gpuHandle);
                }

                if (mesh.material && mesh.material->diffuse)
                {
                    if (mesh.material->diffuse->gpuHandle.IsValid())
                    {
                        //mesh.material->diffuse->gpuHandle = m_graphics.DestroyTextureInstance(*mesh.material->diffuse);
                    }
                }
            }
        }
    }

    void Renderer::Render(const Level& level, const Lara& lara, const Camera& camera)
    {
        m_graphics.ClearFrameBuffer();

        m_graphics.UseShader(m_skinnedShader.gpuHandle);

        m_viewMatrix = glm::inverse(camera.transform.AsMatrix());
        m_graphics.SetMatrix4(m_skinnedLocations.viewMatrix, m_viewMatrix);

        m_projectionMatrix = glm::perspective(camera.fovY, camera.aspect, camera.near, camera.far);
        m_graphics.SetMatrix4(m_skinnedLocations.projectMatrix, m_projectionMatrix);

        m_graphics.SetVec3(m_skinnedLocations.ambientColor, level.ambientColor);
        m_graphics.SetFloat(m_skinnedLocations.ambientIntensity, level.ambientStrength);

        if (m_lightsTexture.gpuHandle.IsValid())
        {
            m_graphics.SetTexture(m_skinnedLocations.lights, m_lightsTexture.gpuHandle, 2);
        }

        Frustum cameraPlanes{};
        ExtractCameraPlanes(cameraPlanes, m_projectionMatrix * m_viewMatrix);

        m_opaqueQueue.clear();
        m_transparentQueue.clear();
        FillRenderQueues(0, level, cameraPlanes, m_opaqueQueue, m_transparentQueue);
        PerformRenderPass(level, m_opaqueQueue, m_transparentQueue);

        if (lara.model)
        {
            MeshLightArray laraLights{};
            GetClosestLights(level, lara.transform.position, laraLights);

            Transform laraFinalTransform = lara.transform;
            laraFinalTransform.rotation = lara.transform.rotation * glm::quat(lara.modelRotationOffset);
            DrawModel(*lara.model.get(), laraFinalTransform, laraLights, false, &lara.animPlayer.FinalBoneMatrices());
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

            if (mesh.material && mesh.material->diffuse)
            {
                if (!mesh.material->diffuse->gpuHandle.IsValid())
                {
                    mesh.material->diffuse->gpuHandle = m_graphics.CreateTextureInstance(*mesh.material->diffuse);
                    if (!mesh.material->diffuse->gpuHandle.IsValid())
                    {
                        LOG_ERROR("Could not initialize texture: %s", mesh.material->diffuse->name.c_str());
                    }
                }
            }
        }

        return true;
    }

    void Renderer::RenderWireframe(const Model& model, const Transform& transform, const Camera& camera)
    {
        m_graphics.UseShader(m_gizmoShader.gpuHandle); // Just does a color - same thing

        m_graphics.ClearDepthBuffer();

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
        m_graphics.UseShader(m_gizmoShader.gpuHandle); // Just does a color - same thing

        m_graphics.ClearDepthBuffer();

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
        lines.reserve(m_octTree.nodes.size() * 12);

        for (const auto& node : m_octTree.nodes)
        {
            // Means we want to draw the nodes that contain index "contains"
            if (contains != UINT32_MAX)
            {
                bool found = false;
                for (uint32_t c : node.contains)
                {
                    if (c == contains)
                        found = true;
                }

                if (!found)
                    continue;
            }

            const glm::vec3& min = node.bounds.min;
            const glm::vec3& max = node.bounds.max;

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

        //m_graphics.UseShader(m_lineShader.gpuHandle);
        m_graphics.UseLineShader();

        const glm::mat4 view = glm::inverse(camera.transform.AsMatrix());
        m_graphics.SetMatrix4("view", view);

        const glm::mat4 projection = glm::perspective(camera.fovY, camera.aspect, camera.near, camera.far);
        m_graphics.SetMatrix4("projection", projection);

        m_graphics.SetMatrix4("model", glm::mat4{ 1.0f });

        m_graphics.DrawLines(lines);
    }

    void Renderer::DrawBox(const AABB& aabb, const glm::vec4& color, const Camera& camera)
    {
        std::vector<Line> lines{};
        lines.reserve(12);

        const glm::vec3& min = aabb.min;
        const glm::vec3& max = aabb.max;

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

        m_graphics.UseShader(m_lineShader.gpuHandle);

        const glm::mat4 view = glm::inverse(camera.transform.AsMatrix());
        m_graphics.SetMatrix4("view", view);

        const glm::mat4 projection = glm::perspective(camera.fovY, camera.aspect, camera.near, camera.far);
        m_graphics.SetMatrix4("projection", projection);

        m_graphics.SetMatrix4("model", glm::mat4{ 1.0f });

        m_graphics.DrawLines(lines);
    }

    void Renderer::InitializeShaders()
    {
        m_skinnedShader.vertexSource = FileIO::ReadEntireFile(SkinnedVertexShaderPath);
        m_skinnedShader.fragmentSource = FileIO::ReadEntireFile(BaseFragmentShaderPath);
        m_graphics.InitializeShader(m_skinnedShader);

        m_lineShader.vertexSource = FileIO::ReadEntireFile(LineVertexShaderPath);
        m_lineShader.fragmentSource = FileIO::ReadEntireFile(ColorFragmentShaderPath);
        m_graphics.InitializeShader(m_lineShader);

        m_gizmoShader.vertexSource = FileIO::ReadEntireFile(BaseVertexShaderPath);
        m_gizmoShader.fragmentSource = FileIO::ReadEntireFile(GizmoFragmentShaderPath);
        m_graphics.InitializeShader(m_gizmoShader);

        m_depthShader.vertexSource = FileIO::ReadEntireFile(DepthVertexShaderPath);
        m_depthShader.fragmentSource = FileIO::ReadEntireFile(GizmoFragmentShaderPath);
        m_graphics.InitializeShader(m_depthShader);

        m_graphics.UseShader(m_skinnedShader.gpuHandle);

        m_skinnedLocations.diffuse = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "diffuseTexture");
        m_skinnedLocations.normal = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "normalTexture");

        m_skinnedLocations.lights = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightsTexture");
        m_skinnedLocations.numLights = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "numLights");
        m_skinnedLocations.lightIndices[0] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[0]");
        m_skinnedLocations.lightIndices[1] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[1]");
        m_skinnedLocations.lightIndices[2] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[2]");
        m_skinnedLocations.lightIndices[3] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[3]");
        m_skinnedLocations.lightIndices[4] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[4]");
        m_skinnedLocations.lightIndices[5] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[5]");
        m_skinnedLocations.lightIndices[6] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[6]");
        m_skinnedLocations.lightIndices[7] = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "lightIndices[7]");

        m_skinnedLocations.modelMatrix = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "model");
        m_skinnedLocations.viewMatrix = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "view");
        m_skinnedLocations.projectMatrix = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "projection");

        m_skinnedLocations.ambientColor = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "ambientColor");
        m_skinnedLocations.ambientIntensity = m_graphics.GetLocation(m_skinnedShader.gpuHandle, "ambientStrength");
    }

    void Renderer::SubmitLightsTexture(const std::vector<PointLight>& lights)
    {
        if (lights.size() < 1)
        {
            return;
        }

        const size_t textureSize = lights.size() * sizeof(PointLight);

        m_lightsTexture.width = lights.size() * 9; // 9 floats make up a point light
        m_lightsTexture.height = 1;
        m_lightsTexture.format = TextureFormat::R;
        m_lightsTexture.type = TextureDataType::Float;
        m_lightsTexture.filter = TextureFilter::Nearest;
        m_lightsTexture.data.resize(textureSize);

        memcpy(m_lightsTexture.data.data(), lights.data(), textureSize);

        m_lightsTexture.gpuHandle = m_graphics.CreateTextureInstance(m_lightsTexture);
    }

    void Renderer::FillRenderQueues(uint32_t nodeIndex,
        const Level& level,
        const Frustum& frustum,
        std::vector<uint32_t>& opaque,
        std::vector<uint32_t>& transparent)
    {
        const auto& node = m_octTree.nodes[nodeIndex];

        if (FrustumIntersectsAABB(frustum, node.bounds))
        {
            for (uint32_t item : node.contains)
            {
                const MeshInstance& obj = level.meshes[item];

                if (obj.mesh)
                {
                    if (obj.mesh->material && obj.mesh->material->TestFlag(MATERIAL_FLAG_TRANSPARENT))
                    {
                        transparent.emplace_back(item);
                    }
                    else
                    {
                        opaque.emplace_back(item);
                    }
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
        for (uint32_t objIndex : opaque)
        {
            m_graphics.SetMatrix4(m_skinnedLocations.modelMatrix, level.meshes[objIndex].modelMatrix);
            DrawMesh(*level.meshes[objIndex].mesh, level.meshes[objIndex].lights);
        }

        for (uint32_t objIndex : transparent)
        {
            m_graphics.SetMatrix4(m_skinnedLocations.modelMatrix, level.meshes[objIndex].modelMatrix);
            DrawMesh(*level.meshes[objIndex].mesh, level.meshes[objIndex].lights);
        }
    }

    void Renderer::SetMaterial(const Material& material)
    {
        if (material.TestFlag(MATERIAL_FLAG_DIFFUSE))
        {
            const auto& diffuse = material.diffuse;
            if (diffuse && diffuse->gpuHandle.IsValid())
            {
                m_graphics.SetTexture(m_skinnedLocations.diffuse, diffuse->gpuHandle, 0);
            }
            else
            {
                m_graphics.SetMagentaTexture("diffuseTexture");
            }
        }
        else
        {
            m_graphics.SetWhiteTexture("diffuseTexture");
        }

        if (material.TestFlag(MATERIAL_FLAG_NORMAL))
        {
            const auto& normal = material.normal;
            if (normal && normal->gpuHandle.IsValid())
            {
                m_graphics.SetTexture(m_skinnedLocations.normal, normal->gpuHandle, 1);
            }
        }
    }

    void Renderer::DrawModel(const Model& model,
        const Transform& transform, 
        const MeshLightArray& lightIndices,
        bool transparentPass,
        const std::vector<glm::mat4>* boneMatrices)
    {
        for (size_t m = 0; m < model.meshes.size(); m++)
        {
            auto& mesh = model.meshes[m];

            if (!mesh.isActive || !mesh.gpuHandle.IsValid())
            {
                continue;
            }

            if (mesh.name.find("PART") != std::string::npos)
            {
                int y = 0;
                (void)y;
            }

            m_graphics.SetMatrix4("model", transform.AsMatrix());

            if (mesh.material)
            {
                const auto& material = mesh.material;

                const bool isTransparent = material->TestFlag(MATERIAL_FLAG_TRANSPARENT);

                const bool shouldDraw = (transparentPass && isTransparent) || (!transparentPass && !isTransparent);
                if (!shouldDraw)
                {
                    continue;
                }

                SetMaterial(*mesh.material);
            }

            if (boneMatrices)
            {
                m_graphics.SetMatrix4Array("finalBonesMatrices", *boneMatrices);
            }

            if (mesh.isDoubleSided)
            {
                glDisable(GL_CULL_FACE);
            }

            DrawMesh(mesh, lightIndices);

            if (mesh.isDoubleSided)
            {
                glEnable(GL_CULL_FACE);
            }
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

    void Renderer::DrawMesh(const Mesh& mesh, const MeshLightArray& lightIndices)
    {
        if (!mesh.isActive)
        {
            return;
        }

        if (mesh.material)
        {
            SetMaterial(*mesh.material);
        }

        for (size_t i = 0; i < lightIndices.size(); i++)
        {
            ShaderLocation location;
            switch (i)
            {
            case 0:
                location = m_skinnedLocations.lightIndices[0];
                break;
            case 1:
                location = m_skinnedLocations.lightIndices[1];
                break;
            case 2:
                location = m_skinnedLocations.lightIndices[2];
                break;
            case 3:
                location = m_skinnedLocations.lightIndices[3];
                break;
            case 4:
                location = m_skinnedLocations.lightIndices[4];
                break;
            case 5:
                location = m_skinnedLocations.lightIndices[5];
                break;
            case 6:
                location = m_skinnedLocations.lightIndices[6];
                break;
            case 7:
                location = m_skinnedLocations.lightIndices[7];
                break;
            default:
                break;
            }
            m_graphics.SetInt(location, lightIndices[i]);
        }
        m_graphics.SetInt(m_skinnedLocations.numLights, lightIndices.size() < 8 ? lightIndices.size() : 8);

        if (mesh.isDoubleSided)
        {
            glDisable(GL_CULL_FACE);
        }

        m_graphics.DrawMesh(mesh.gpuHandle);

        if (mesh.isDoubleSided)
        {
            glEnable(GL_CULL_FACE);
        }
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
            if (!obj.mesh)
            {
                continue;
            }

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
