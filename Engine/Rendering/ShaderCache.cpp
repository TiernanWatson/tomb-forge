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

        const std::string staticVertex = FileIO::ReadEntireFile(StaticVertexShaderPath);
        const std::string pbrFragment = FileIO::ReadEntireFile(PBRFragmentShaderPath);
        if (m_staticMeshShader = graphics.CompileShader(staticVertex.c_str(), pbrFragment.c_str()); !m_staticMeshShader.IsInitialized())
        {
            LOG_ERROR("Could not compile static mesh shader");
            return false;
        }

        const std::string skinnedVertex = FileIO::ReadEntireFile(SkinnedVertexShaderPath);
        if (m_skinnedMeshShader = graphics.CompileShader(skinnedVertex.c_str(), pbrFragment.c_str()); !m_skinnedMeshShader.IsInitialized())
        {
            LOG_ERROR("Could not compile the skinned mesh renderer");
            return false;
        }

        const std::string lineVertex = FileIO::ReadEntireFile(LineVertexShaderPath);
        const std::string colorFragment = FileIO::ReadEntireFile(ColorFragmentShaderPath);
        if (m_lineShader = graphics.CompileShader(lineVertex.c_str(), colorFragment.c_str()); !m_lineShader.IsInitialized())
        {
            LOG_ERROR("Could not compile line shader");
            return false;
        }

        const std::string gizmoFragment = FileIO::ReadEntireFile(GizmoFragmentShaderPath);
        if (m_gizmoShader = graphics.CompileShader(staticVertex.c_str(), gizmoFragment.c_str()); !m_gizmoShader.IsInitialized())
        {
            LOG_ERROR("Could not compile gizmo shader");
            return false;
        }
    }
}
