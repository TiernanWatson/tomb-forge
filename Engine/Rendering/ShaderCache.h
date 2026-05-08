#pragma once

#include <unordered_map>

#include "Engine/Rendering/ShaderInstance.h"

namespace TombForge
{
    // Shared class that allows different types of renderers to use the same shaders
    class ShaderCache
    {
    public:
        bool Initialize();

        ShaderInstance* GetOrCreateShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);

        ShaderInstance* GetShader(const std::string& name)
        {
            if (m_shaderCache.contains(name))
            {
                return &m_shaderCache[name];
            }

            return nullptr;
        }

        ShaderInstance* GetStaticMeshShader() const { return m_staticMeshShader; }
        ShaderInstance* GetSkinnedMeshShader() const { return m_skinnedMeshShader; }
        ShaderInstance* GetDepthShader() const { return m_depthShader; }
        ShaderInstance* GetLineShader() const { return m_lineShader; }
        ShaderInstance* GetGizmoShader() const { return m_gizmoShader; }

        static ShaderCache& Get()
        {
            static ShaderCache cache{};
            return cache;
        }

    private:
        ShaderInstance* m_staticMeshShader{};
        ShaderInstance* m_skinnedMeshShader{};
        ShaderInstance* m_depthShader{};
        ShaderInstance* m_lineShader{};
        ShaderInstance* m_gizmoShader{};

        std::unordered_map<std::string, ShaderInstance> m_shaderCache{};
    };
}

