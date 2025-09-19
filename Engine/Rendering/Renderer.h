#pragma once

#include <memory>
#include <vector>

#include <glm/detail/type_vec.hpp>
#include <unordered_set>

#include "Core/Graphics/AABB.h"
#include "Core/Graphics/Frustum.h"
#include "Core/Maths/Transform.h"
#include "Engine/Levels/Level.h"
#include "Engine/Rendering/Graphics.h"
#include "Engine/Rendering/Shader.h"
#include "Engine/Rendering/Texture.h"

namespace TombForge
{
    struct Model;
    struct Lara;
    struct Camera;

    class AnimPlayer;

    enum OctDirection : uint8_t
    {
        OCT_TOP_FORWARD_LEFT,
        OCT_TOP_FORWARD_RIGHT,
        OCT_TOP_BACK_LEFT,
        OCT_TOP_BACK_RIGHT,
        OCT_BOTTOM_FORWARD_LEFT,
        OCT_BOTTOM_FORWARD_RIGHT,
        OCT_BOTTOM_BACK_LEFT,
        OCT_BOTTOM_BACK_RIGHT
    };

    struct OctNode
    {
        // Indexed by static object index
        std::vector<uint32_t> contains{};

        // World-space AABB of this node
        AABB bounds{};

        // Child node indices in vector - 0 means null
        uint32_t subNodeIndices[8]{};

        // Irrelevant if index 0 in array
        uint32_t parent{};
    };

    struct OctTree
    {
        std::vector<OctNode> nodes{};

        bool Build(const Level& level, const float minNodeSize = 10.0f);
        bool Insert(uint32_t objIndex, const AABB& aabb, const float minNodeSize = 5.0f, uint32_t nodeIndex = 0, uint8_t depth = 0);
    };

    /// <summary>
    /// System that renders a level every frame
    /// </summary>
    class Renderer
    {
    public:
        Renderer();

        // When a level is loaded, call this to set it up with the renderer
        bool InitializeLevel(Level& level);

        // This destroys all the GPU instances of the meshes when a level is unloaded
        void DeloadLevel(Level& level);

        // Updates the lights texture on the GPU whenever they change
        void UpdateLights(Level& level);

        // To be called before rendering a frame
        void ClearFramebuffer();

        // Called once per frame to render the level
        void RenderLevel(const Level& level, const Lara& lara, const Camera& camera);

        // Renders just Lara (useful for debugging as well as normal rendering)
        void RenderLara(const Level& level, const Lara& lara, const Camera& camera);

        // Sets the camera view and projection matrices in the shader
        void SetCamera(const Transform& transform, float fovY, float aspect, float near, float far);

        // Called whenever the framebuffer needs to be resized
        void OnWindowResized(int width, int height);

        // Sets up a CPU model on the GPU
        bool InitializeModel(Model& model);

        // Renders the specified model as wireframe
        void RenderWireframe(const Model& model, const Transform& transform, const Camera& camera);

        // Renders a mesh as wireframe
        void RenderWireframe(const Mesh& model, const Transform& transform, const Camera& camera);

#if EDITOR_ENABLED
        void DrawOctree(const glm::vec4& color, const Camera& camera, uint32_t contains = UINT32_MAX);

        void DrawBox(const AABB& aabb, const glm::vec4& color, const Camera& camera);
#endif

    private:
        struct ShaderLocations
        {
            ShaderLocation diffuse{};
            ShaderLocation normal{};

            ShaderLocation lights{};
            ShaderLocation lightIndices[8]{};
            ShaderLocation numLights{};

            ShaderLocation modelMatrix{};
            ShaderLocation viewMatrix{};
            ShaderLocation projectMatrix{};

            ShaderLocation ambientColor{};
            ShaderLocation ambientIntensity{};

            ShaderLocation dirLightColor{};
            ShaderLocation dirLightDirection{};
            ShaderLocation dirLightIntensity{};
        };

        void InitializeShaders();

        void InitializeDefaultTextures();

        void SubmitLightsTexture(const std::vector<PointLight>& lights);

        void FillRenderQueues(uint32_t nodeIndex, 
            const Level& level, 
            const Frustum& frustum, 
            std::vector<uint32_t>& opaque, 
            std::vector<uint32_t>& transparent);

        void PerformRenderPass(const Level& level, std::vector<uint32_t>& opaque, std::vector<uint32_t>& transparent);

        void SetMaterial(const Material& material);

        void DrawModel(const Model& model, 
            const Transform& transform, 
            const MeshLightArray& lightIndices,
            bool transparentPass = false,
            const std::vector<glm::mat4>* boneMatrices = nullptr);

        void DrawModelDepth(const Model& model, const Transform& transform);

        void DrawMesh(const Mesh& mesh, const MeshLightArray& lightIndices, const Material* overrideMaterial = nullptr);

        void ExtractCameraPlanes(Frustum& result, const glm::mat4& viewProj) const;

        OctTree m_octTree{};

        Shader m_baseShader{};
        Shader m_skinnedShader{};
        Shader m_lineShader{};
        Shader m_gizmoShader{};
        Shader m_depthShader{};

        Texture m_lightsTexture{};
        Texture m_whiteTexture{};
        Texture m_magentaTexture{};
        Texture m_flatNormal{};

        ShaderLocations m_skinnedLocations{};

        glm::mat4 m_viewMatrix{};
        glm::mat4 m_projectionMatrix{};

        std::vector<uint32_t> m_opaqueQueue{};
        std::vector<uint32_t> m_transparentQueue{};

        Graphics& m_graphics;
    };
}

