#pragma once

#include <string>
#include <vector>

#include "Engine/Rendering/Graphics.h"

namespace TombForge
{
    // This represents a compiled shader and allows for caching uniform locations
    class ShaderInstance
    {
    public:
        ShaderInstance() = default;

        bool Compile(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);

        ShaderLocation GetLocation(const std::string& name) const; // This should not typically be called every frame

        void CacheLocations(const std::initializer_list<std::string>& names);

        inline ShaderHandle GetHandle() const { return m_gpuHandle; }

        inline const std::string& GetName() const { return m_name; }

    private:
        struct CachedLocation
        {
            std::string name{};
            ShaderLocation location{};
        };

        ShaderHandle m_gpuHandle{};
        std::vector<CachedLocation> m_locationCache{};
        std::string m_name{};
    };
}

