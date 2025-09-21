#pragma once

#include "Engine/Rendering/Graphics.h"

namespace TombForge
{
    // Shared class that allows different types of renderers to use the same shaders
    class ShaderCache
    {
    public:
        bool Initialize();

        ShaderHandle GetStaticMeshShader() const { return m_staticMeshShader; }
        ShaderHandle GetSkinnedMeshShader() const { return m_skinnedMeshShader; }
        ShaderHandle GetDepthShader() const { return m_depthShader; }
        ShaderHandle GetLineShader() const { return m_lineShader; }
        ShaderHandle GetGizmoShader() const { return m_gizmoShader; }

        static ShaderCache& Get()
        {
            static ShaderCache cache{};
            return cache;
        }

    private:
        ShaderHandle m_staticMeshShader{};
        ShaderHandle m_skinnedMeshShader{};
        ShaderHandle m_depthShader{};
        ShaderHandle m_lineShader{};
        ShaderHandle m_gizmoShader{};
    };
}

