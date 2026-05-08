#include "ShaderCache.h"

#include <string>

#include "Core/Debug.h"
#include "Core/IO/FileIO.h"

namespace TombForge
{
    namespace
    {
        constexpr char const* SkinnedVertexShaderPath = "Shaders\\SkinnedMesh.vert";
        constexpr char const* PBRFragmentShaderPath = "Shaders\\ForwardLit_PBR.frag";
        constexpr char const* StaticVertexShaderPath = "Shaders\\StaticMesh.vert";
        constexpr char const* LineVertexShaderPath = "Shaders\\PositionColor.vert";
        constexpr char const* ColorFragmentShaderPath = "Shaders\\VertexColorUnlit.frag";
        constexpr char const* GizmoFragmentShaderPath = "Shaders\\UniformColorUnlit.frag";
    }

    bool ShaderCache::Initialize()
    {
        auto& graphics = Graphics::Get();

        bool allCompiled = true;

        const std::string staticVertex = FileIO::ReadEntireFile(StaticVertexShaderPath);
        const std::string pbrFragment = FileIO::ReadEntireFile(PBRFragmentShaderPath);
        m_staticMeshShader = GetOrCreateShader("StaticMesh_Lit", staticVertex, pbrFragment);
        if (m_staticMeshShader == nullptr || !m_staticMeshShader->GetHandle().IsInitialized())
        {
            LOG_ERROR("Could not compile static mesh shader");
            allCompiled = false;
        }

        const std::string skinnedVertex = FileIO::ReadEntireFile(SkinnedVertexShaderPath);
        m_skinnedMeshShader = GetOrCreateShader("SkinnedMesh_Lit", skinnedVertex, pbrFragment);
        if (m_skinnedMeshShader == nullptr || !m_skinnedMeshShader->GetHandle().IsInitialized())
        {
            LOG_ERROR("Could not compile the skinned mesh renderer");
            allCompiled = false;
        }

        const std::string lineVertex = FileIO::ReadEntireFile(LineVertexShaderPath);
        const std::string colorFragment = FileIO::ReadEntireFile(ColorFragmentShaderPath);
        m_lineShader = GetOrCreateShader("LineRenderer", lineVertex, colorFragment);
        if (m_lineShader == nullptr || !m_lineShader->GetHandle().IsInitialized())
        {
            LOG_ERROR("Could not compile line shader");
            allCompiled = false;
        }

        const std::string gizmoFragment = FileIO::ReadEntireFile(GizmoFragmentShaderPath);
        m_gizmoShader = GetOrCreateShader("GizmoRenderer", staticVertex, gizmoFragment);
        if (m_gizmoShader == nullptr || !m_gizmoShader->GetHandle().IsInitialized())
        {
            LOG_ERROR("Could not compile gizmo shader");
            allCompiled = false;
        }

        return allCompiled;
    }

    ShaderInstance* ShaderCache::GetOrCreateShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
    {
        if (m_shaderCache.contains(name))
        {
            return &m_shaderCache[name];
        }

        auto& instance = m_shaderCache[name];
        instance.Compile(name, vertexSource, fragmentSource);
        if (!instance.GetHandle().IsInitialized())
        {
            LOG_ERROR("Failed to compile shader: %s", name.c_str());
        }

        return &instance;
    }
}
