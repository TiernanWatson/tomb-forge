#include "Graphics.h"

#include <stdexcept>

#include <glad/glad.h>
#include <glfw3.h>

#include "../Debug.h"
#include "Model.h"
#include "../../Core/IO/FileIO.h"
#include "Texture.h"
#include "Shader.h"

#include <intrin.h>

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

    static bool s_graphicsInitialized{ false };

    uint32_t MeshHandle::InvalidMeshIndex = (uint32_t)-1;
    uint32_t TextureHandle::InvalidTextureIndex = (uint32_t)-1;

    // Generic function to test if any errors occurred
    void LoopGLErrors()
    {
        GLenum error{};
        for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        {
            switch (error)
            {
            case GL_INVALID_ENUM:
                LOG_ERROR("GL Error invalid enum");
                break;
            case GL_INVALID_VALUE:
                LOG_ERROR("GL Error invalid value");
                break;
            case GL_INVALID_OPERATION:
                LOG_ERROR("GL Error invalid operation");
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                LOG_ERROR("GL Error invalid framebuffer operation");
                break;
            case GL_OUT_OF_MEMORY:
                LOG_ERROR("GL Error out of memory");
                break;
            case GL_STACK_UNDERFLOW:
                LOG_ERROR("GL Error stack underflow");
                break;
            case GL_STACK_OVERFLOW:
                LOG_ERROR("GL Error stack overflow");
                break;
            default:
                LOG_ERROR("Undefined GL error");
                break;
            }
        }
    }

    Graphics::Graphics()
    {
        if (s_graphicsInitialized)
        {
            throw std::runtime_error("Trying to initialize GLAD twice");
        }

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        const std::string vertexSource = FileIO::ReadEntireFile(SkinnedVertexShaderPath);
        const std::string fragSource = FileIO::ReadEntireFile(BaseFragmentShaderPath);

        if (!CompileShader(vertexSource, fragSource, m_skinnedShader))
        {
            throw std::runtime_error("Failed to compile default shader");
        }

        const std::string lineVertexSource = FileIO::ReadEntireFile(LineVertexShaderPath);
        const std::string colorFragmentSource = FileIO::ReadEntireFile(ColorFragmentShaderPath);

        if (!CompileShader(lineVertexSource, colorFragmentSource, m_lineShader))
        {
            throw std::runtime_error("Failed to compile line shader");
        }

        const std::string gizmoFragmentSource = FileIO::ReadEntireFile(GizmoFragmentShaderPath);
        const std::string baseVertexSource = FileIO::ReadEntireFile(BaseVertexShaderPath);

        if (!CompileShader(baseVertexSource, gizmoFragmentSource, m_gizmoShader))
        {
            throw std::runtime_error("Failed to compile gizmo shader");
        }

        const std::string depthFragmentSource = FileIO::ReadEntireFile(DepthFragmentShaderPath);
        const std::string depthVertexSource = FileIO::ReadEntireFile(DepthVertexShaderPath);

        if (!CompileShader(depthVertexSource, ""/*depthFragmentSource*/, m_depthShader))
        {
            throw std::runtime_error("Failed to compile depth shader");
        }

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_MULTISAMPLE);

        // Setup white texture
        m_whiteTexture = std::make_unique<Texture>();
        m_whiteTexture->width = m_whiteTexture->height = 1;
        m_whiteTexture->format = TextureFormat::RGBA;

        ColorByte white = 255;
        m_whiteTexture->data.emplace_back(white);
        m_whiteTexture->data.emplace_back(white);
        m_whiteTexture->data.emplace_back(white);
        m_whiteTexture->data.emplace_back(white);

        m_whiteTexture->gpuHandle = CreateTextureInstance(*m_whiteTexture);

        // Setup (error) magenta texture
        m_magentaTexture = std::make_unique<Texture>();
        m_magentaTexture->width = m_magentaTexture->height = 1;
        m_magentaTexture->format = TextureFormat::RGB;
        m_magentaTexture->data.emplace_back(white);
        m_magentaTexture->data.emplace_back(0);
        m_magentaTexture->data.emplace_back(white);

        m_magentaTexture->gpuHandle = CreateTextureInstance(*m_magentaTexture);

        s_graphicsInitialized = true;

        LoopGLErrors();
    }

    Graphics::~Graphics()
    {
        glDeleteProgram(m_skinnedShader);
        glDeleteProgram(m_lineShader);
    }

    Graphics& Graphics::Get()
    {
        static Graphics graphics{};
        return graphics;
    }

    void Graphics::SetDepthTest(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }

    MeshHandle Graphics::CreateMeshInstance(const Mesh& mesh)
    {
        if (m_meshes.size() >= (MeshHandle::InvalidMeshIndex - 1))
        {
            return MeshHandle{};
        }

        GLuint vao{};
        GLuint vbo{};
        GLuint ebo{};

        // Generate buffers

        glGenVertexArrays(1, &vao);

        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        // Upload buffer data

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), mesh.indices.data(), GL_STATIC_DRAW);

        // Setup vertex layout

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIndices));

        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

        LoopGLErrors();

        m_meshes.emplace_back(mesh.indices.size(), vao, vbo, ebo);

        return { static_cast<uint32_t>(m_meshes.size() - 1) };
    }

    void Graphics::DestroyMeshInstance(MeshHandle& instance)
    {
        if (!instance.IsValid())
        {
            return;
        }

        MeshInstance& mesh = m_meshes[instance.index];

        glDeleteBuffers(1, &mesh.ibo);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.vao);

        instance.index = MeshHandle::InvalidMeshIndex;
    }

    void Graphics::DrawMesh(MeshHandle mesh)
    {
        if (!mesh.IsValid())
        {
            return;
        }

        MeshInstance& instance = m_meshes[mesh.index];

        glBindVertexArray(instance.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(instance.indicesCount), GL_UNSIGNED_INT, 0);

        LoopGLErrors();
    }

    void Graphics::DrawLines(const std::vector<Line>& lines)
    {
        if (lines.size() == 0)
        {
            return;
        }

        GLuint vao{};
        GLuint vbo{};

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);

        // Upload buffer data

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Line) * lines.size(), lines.data(), GL_STATIC_DRAW);

        // Setup vertex layout

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, point));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));

        glUseProgram(m_lineShader);

        // * 2 because the Line struct is made of two points and we send individual points to the shader
        glDrawArrays(GL_LINES, 0, lines.size() * 2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);

        LoopGLErrors();
    }

    void Graphics::DrawTriangle(const Triangle& triangle)
    {
        unsigned int vao, vbo;

        glGenVertexArrays(1, &vao);

        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        // Upload buffer data

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Triangle), &triangle, GL_STATIC_DRAW);

        // Setup vertex layout

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)offsetof(Triangle, p0));

        glUseProgram(m_lineShader);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &vao);
    }

    TextureHandle Graphics::CreateTextureInstance(const Texture& texture)
    {
        if (m_textures.size() >= (TextureHandle::InvalidTextureIndex - 1) || texture.data.size() == 0)
        {
            return TextureHandle{};
        }

        // Create or re-use

        unsigned int id{};
        size_t arrayIndex{};
        if (texture.gpuHandle.IsValid())
        {
            arrayIndex = texture.gpuHandle.index;
            id = m_textures[texture.gpuHandle.index];
        }
        else
        {
            glGenTextures(1, &id);
            m_textures.emplace_back(id);
            arrayIndex = m_textures.size() - 1;
        }

        glBindTexture(GL_TEXTURE_2D, id);

        // Format details

        const bool isFloats = texture.type == TextureDataType::Float;

        GLenum format;
        GLenum internalFormat;
        switch (texture.format)
        {
        case TextureFormat::R:
            format = GL_RED;
            internalFormat = isFloats ? GL_R32F : GL_R8;
            break;
        case TextureFormat::RGB:
            format = GL_RGB;
            internalFormat = isFloats ? GL_RGB32F : GL_RGB8;
            break;
        case TextureFormat::RGBA:
            format = GL_RGBA;
            internalFormat = isFloats ? GL_RGBA32F : GL_RGBA8;
            break;
        default:
            return TextureHandle{};
        }

        const GLenum type = isFloats ? GL_FLOAT : GL_UNSIGNED_BYTE;

        GLenum minFilter;
        GLenum maxFilter;
        switch (texture.filter)
        {
        case TextureFilter::Nearest:
            minFilter = GL_NEAREST;
            maxFilter = GL_NEAREST;
            break;
        case TextureFilter::Bilinear:
            minFilter = GL_LINEAR;
            maxFilter = GL_LINEAR;
            break;
        case TextureFilter::Trilinear:
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
            maxFilter = GL_LINEAR;
            break;
        default:
            LOG_ERROR("Unsupported texture filter %i", texture.filter);
            return TextureHandle{};
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maxFilter);

        // Send data

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texture.width, texture.height, 0, format, type, (void*)texture.data.data());

        if (texture.filter == TextureFilter::Trilinear)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        LoopGLErrors();

        return TextureHandle{ static_cast<uint32_t>(arrayIndex) };
    }

    bool Graphics::CompileShader(const std::string& vertexSource, const std::string& fragmentSource, unsigned int& outProgramId)
    {
        unsigned int vertexShader; 
        if (!CompileShaderPart(vertexSource.c_str(), GL_VERTEX_SHADER, vertexShader))
        {
            return false;
        }

        const bool compileFragment = fragmentSource.size() > 0;

        unsigned int fragmentShader; 
        if (compileFragment)
        {
            if (!CompileShaderPart(fragmentSource.c_str(), GL_FRAGMENT_SHADER, fragmentShader))
            {
                return false;
            }
        }

        outProgramId = glCreateProgram();
        glAttachShader(outProgramId, vertexShader);
        if (compileFragment)
        {
            glAttachShader(outProgramId, fragmentShader);
        }
        glLinkProgram(outProgramId);

        GLint linkStatus{};
        char infoLog[1024]{};
        glGetProgramiv(outProgramId, GL_LINK_STATUS, &linkStatus);
        if (!linkStatus)
        {
            glGetProgramInfoLog(outProgramId, 1024, NULL, infoLog);
            LOG_ERROR("Shader Link Error: %s", infoLog);
            __debugbreak();
            return false;
        }

        glDeleteShader(vertexShader);
        if (compileFragment)
        {
            glDeleteShader(fragmentShader);
        }

        LoopGLErrors();

        return true;
    }

    void Graphics::SetFloat(const std::string& name, float value)
    {
        GLuint id = glGetUniformLocation(m_activeShader, name.c_str());
        glUniform1f(id, value);
    }

    void Graphics::SetFloat(ShaderLocation location, float value)
    {
        glUniform1f(location, value);
    }

    void Graphics::SetVec3(const std::string& name, glm::vec3 value)
    {
        GLuint id = glGetUniformLocation(m_activeShader, name.c_str());
        glUniform3f(id, value.x, value.y, value.z);
    }

    void Graphics::SetVec3(ShaderLocation location, const glm::vec3& value)
    {
        glUniform3f(location, value.x, value.y, value.z);
    }

    void Graphics::SetVec4(const std::string& name, glm::vec4 value)
    {
        GLuint id = glGetUniformLocation(m_activeShader, name.c_str());
        glUniform4f(id, value.x, value.y, value.z, value.w);
    }

    void Graphics::SetVec4(ShaderLocation location, const glm::vec4& value)
    {
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void Graphics::SetInt(const std::string& name, int value)
    {
        GLuint id = glGetUniformLocation(m_activeShader, name.c_str());
        glUniform1i(id, value);
    }

    void Graphics::SetInt(ShaderLocation location, int value)
    {
        glUniform1i(location, value);
        LoopGLErrors();
    }

    void Graphics::SetMatrix4(const std::string& name, const glm::mat4& matrix, GLuint programId)
    {
        GLuint id = glGetUniformLocation(programId, name.c_str());
        glUniformMatrix4fv(id, 1, GL_FALSE, &matrix[0][0]);
    }

    void Graphics::SetMatrix4(ShaderLocation location, const glm::mat4& value)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    }

    void Graphics::SetMatrix4Array(const std::string& name, const std::vector<glm::mat4>& items)
    {
        GLuint id = glGetUniformLocation(m_activeShader, name.c_str());
        glUniformMatrix4fv(id, items.size(), GL_FALSE, &(items[0][0][0]));
    }

    void Graphics::SetMatrix4Array(ShaderLocation location, const std::vector<glm::mat4>& items)
    {
        glUniformMatrix4fv(location, items.size(), GL_FALSE, &(items[0][0][0]));
    }

    void Graphics::SetTexture(const std::string& name, const TextureHandle handle)
    {
        glActiveTexture(GL_TEXTURE0);

        GLuint location = glGetUniformLocation(m_activeShader, name.c_str());
        glUniform1i(location, 0);

        glBindTexture(GL_TEXTURE_2D, m_textures[handle.index]);
    }

    void Graphics::SetTexture(ShaderLocation location, const TextureHandle handle)
    {
        glActiveTexture(GL_TEXTURE0);

        glUniform1i(location, 0);

        glBindTexture(GL_TEXTURE_2D, m_textures[handle.index]);

        LoopGLErrors();
    }

    void Graphics::SetTexture(ShaderLocation location, const TextureHandle handle, int unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);

        LoopGLErrors();

        glUniform1i(location, unit);

        LoopGLErrors();

        glBindTexture(GL_TEXTURE_2D, m_textures[handle.index]);

        LoopGLErrors();
    }

    void Graphics::SetWhiteTexture(const std::string& name)
    {
        SetTexture(name, m_whiteTexture->gpuHandle);
    }

    void Graphics::SetMagentaTexture(const std::string& name)
    {
        SetTexture(name, m_magentaTexture->gpuHandle);
    }

    void Graphics::ResizeFramebuffer(int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void Graphics::ClearFrameBuffer()
    {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Graphics::ClearDepthBuffer()
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void Graphics::SetDepthWriteStatus(bool value)
    {
        glDepthMask(value ? 1 : 0);
        LoopGLErrors();
    }

    void Graphics::SetColorWriteStatus(bool value)
    {
        if (value)
        {
            glColorMask(1, 1, 1, 1);
        }
        else
        {
            glColorMask(0, 0, 0, 0);
        }
        LoopGLErrors();
    }

    void Graphics::SetDepthFunc(DepthFunc func)
    {
        switch (func)
        {
        case DepthFunc::Less:
            glDepthFunc(GL_LESS);
            break;
        case DepthFunc::LEqual:
            glDepthFunc(GL_LEQUAL);
            break;
        case DepthFunc::Equal:
            glDepthFunc(GL_EQUAL);
            break;
        default:
            LOG_ERROR("Unsupported depth func %i", func);
            break;
        }
        LoopGLErrors();
    }

    void Graphics::InitializeShader(Shader& shader)
    {
        GLuint programId{};
        if (CompileShader(shader.vertexSource, shader.fragmentSource, programId))
        {
            m_shaders.emplace_back(programId);
            shader.gpuHandle.index = m_shaders.size() - 1;
            shader.gpuHandle.generation = 1;
        }
    }

    ShaderLocation Graphics::GetLocation(ShaderHandle shader, const std::string& name)
    {
        ASSERT(shader.IsInitialized(), "Trying to get uniform location of uninitialized shader");

        return ShaderLocation(glGetUniformLocation(m_shaders[shader.index], name.c_str()));
    }

    void Graphics::UseShader(ShaderHandle handle)
    {
        ASSERT(handle.IsInitialized(), "Trying to use uninitialized shader handle");
        ASSERT(handle.index < m_shaders.size(), "Out-of-range shader index in handle");

        glUseProgram(m_shaders[handle.index]);
        m_activeShader = m_shaders[handle.index];
    }

    bool Graphics::CompileShaderPart(const char* source, unsigned int type, unsigned int& outProgramId) const
    {
        outProgramId = glCreateShader(type);
        glShaderSource(outProgramId, 1, &source, 0);
        glCompileShader(outProgramId);

        int success;
        char infoLog[1024];
        glGetShaderiv(outProgramId, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            glGetShaderInfoLog(outProgramId, 1024, 0, infoLog);
            LOG_ERROR("Failed shader part: %s", infoLog);
            __debugbreak();
            return false;
        }

        return true;
    }
}
