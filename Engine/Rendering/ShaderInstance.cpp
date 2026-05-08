#include "ShaderInstance.h"

#include "Core/Debug.h"

namespace TombForge
{
    bool ShaderInstance::Compile(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
    {
        if (m_gpuHandle.IsInitialized())
        {
            LOG_WARNING("Shader %s is already compiled, recompiling it.", m_name.c_str());
            // TODO DESTROY old shader handle
        }

        m_name = name;
        m_gpuHandle = Graphics::Get().CompileShader(vertexSource.c_str(), fragmentSource.c_str());
        return m_gpuHandle.IsInitialized();
    }

    ShaderLocation ShaderInstance::GetLocation(const std::string& name) const
    {
        for (const auto& cached : m_locationCache)
        {
            if (cached.name == name)
            {
                return cached.location;
            }
        }

        const ShaderLocation location = m_gpuHandle.IsInitialized() ? Graphics::Get().GetLocation(m_gpuHandle, name) : -1;
        if (location == -1)
        {
            LOG_ERROR("Could not find location %s in shader %s", name.c_str(), m_name.c_str());
            return -1;
        }

        return location;
    }

    void ShaderInstance::CacheLocations(const std::initializer_list<std::string>& names)
    {
        ASSERT(m_gpuHandle.IsInitialized(), "Cannot cache locations for uninitialized shader %s", m_name.c_str());
        for (const std::string& name : names)
        {
            const ShaderLocation location = m_gpuHandle.IsInitialized() ? Graphics::Get().GetLocation(m_gpuHandle, name) : -1;
            if (location == -1)
            {
                LOG_ERROR("Could not find location %s in shader %s", name.c_str(), m_name.c_str());
            }
            m_locationCache.emplace_back(CachedLocation{ name, location });
        }
    }
}
