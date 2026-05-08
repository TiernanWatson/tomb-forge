#pragma once

#include <string>
#include <memory>
#include <vector>

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Core/Graphics/Line.h"

namespace TombForge
{
    struct Shader;
    struct Model;
    struct Mesh;
    struct Texture;

    struct GenericHandle
    {
        uint16_t index{}; // Up to 65,535 instances
        uint16_t generation{};

        bool IsInitialized() const
        {
            return generation != 0;
        }
    };

    using ShaderHandle = GenericHandle;
    using LocationHandle = GenericHandle;
    using UboHandle = GenericHandle;

    struct MeshHandle
    {
        static uint32_t InvalidMeshIndex;

        uint32_t index{ InvalidMeshIndex };

        bool IsValid() const
        {
            return index != InvalidMeshIndex;
        }
    };

    struct TextureHandle
    {
        static uint32_t InvalidTextureIndex;

        uint32_t index{ InvalidTextureIndex };

        inline bool IsValid() const
        {
            return index != InvalidTextureIndex;
        }
    };

    struct Triangle
    {
        glm::vec3 p0{};
        glm::vec3 p1{};
        glm::vec3 p2{};
    };

    enum class DepthFunc : uint8_t
    {
        Less,
        LEqual,
        Equal,
    };

    using ShaderLocation = GLuint;

    /// <summary>
    /// For interacting with the graphics API, but doesn't control rendering flow
    /// </summary>
    class Graphics
    {
    public:
        static Graphics& Get();

        void Initialize(int width, int height);

        // Pipeline Settings

        void SetDepthTest(bool enabled);

        // Mesh Functions

        MeshHandle CreateMeshInstance(const Mesh& mesh);
        void DestroyMeshInstance(MeshHandle& instance);

        void DrawMesh(MeshHandle mesh);
        void DrawLines(const std::vector<Line>& lines);
        void DrawTriangle(const Triangle& triangle);

        // Texture Functions

        TextureHandle CreateTextureInstance(const Texture& texture);
        void DestroyTextureInstance(TextureHandle& handle);

        // Shader Functions

        bool CompileShader(const std::string& vertexSource, const std::string& fragmentSource, unsigned int& outProgramId);

        void SetFloat(const std::string& name, float value);
        void SetFloat(ShaderLocation location, float value);

        void SetVec3(const std::string& name, glm::vec3 value);
        void SetVec3(ShaderLocation location, const glm::vec3& value);

        void SetVec4(const std::string& name, glm::vec4 value);
        void SetVec4(ShaderLocation location, const glm::vec4& value);

        void SetInt(const std::string& name, int value);
        void SetInt(ShaderLocation location, int value);

        void SetMatrix4(const std::string& name, const glm::mat4& matrix, GLuint programId);
        inline void SetMatrix4(const std::string& name, const glm::mat4& matrix)
        {
            SetMatrix4(name, matrix, m_activeShader);
        }
        void SetMatrix4(ShaderLocation location, const glm::mat4& value);

        void SetMatrix4Array(const std::string& name, const std::vector<glm::mat4>& items);
        void SetMatrix4Array(ShaderLocation location, const std::vector<glm::mat4>& items);

        void SetTexture(const std::string& name, const TextureHandle handle);
        void SetTexture(ShaderLocation location, const TextureHandle handle);
        void SetTexture(ShaderLocation location, const TextureHandle handle, int unit);

        // UBO Functions

        UboHandle CreateUbo();
        void UpdateUbo(UboHandle& handle, const void* data, size_t size);
        void BindUbo(const UboHandle& handle, GLuint bindingPoint);
        void DestroyUbo(UboHandle& handle);

        // Buffer and pipeline

        void ResizeFramebuffer(int width, int height);

        void ClearFrameBuffer();
        void ClearDepthBuffer();

        void SetDepthWriteStatus(bool value);
        void SetColorWriteStatus(bool value);
        void SetDepthFunc(DepthFunc func);
        void SetFaceCulling(bool enabled);

        // Shaders

        ShaderHandle CompileShader(const char* vertex, const char* fragment);
        ShaderLocation GetLocation(ShaderHandle shader, const std::string& name);
        void UseShader(ShaderHandle handle);

    private:
        struct MeshInstance
        {
            size_t indicesCount{};
            GLuint vao{};
            GLuint vbo{};
            GLuint ibo{};
        };

        Graphics() = default;

        bool CompileShaderPart(const char* source, unsigned int type, unsigned int& outProgramId) const;

        std::vector<MeshInstance> m_meshes{};
        std::vector<GLuint> m_shaders{};
        std::vector<GLuint> m_textures{};
        std::vector<GLuint> m_ubos{};

        GLuint m_activeShader{};
    };
}

