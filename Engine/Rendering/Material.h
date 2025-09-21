#pragma once

#include <glm/vec4.hpp>

#include <cstdint>
#include <memory>

#include "Engine/Rendering/Texture.h"
#include "Engine/Rendering/Graphics.h"

namespace TombForge
{
    enum MaterialFlags : uint8_t
    {
        MATERIAL_FLAG_NONE = 0,
        MATERIAL_FLAG_TRANSPARENT = 1 << 0,
    };

    struct Material : public AssetBase
    {
        static const glm::vec4 DefaultAlbedoColor;
        static const float DefaultRoughness;
        static const float DefaultMetalness;

        std::shared_ptr<Texture> albedoTexture{};
        std::shared_ptr<Texture> normalTexture{};
        std::shared_ptr<Texture> roughnessTexture{};
        std::shared_ptr<Texture> metalnessTexture{};

        glm::vec4 albedoColor{ DefaultAlbedoColor }; // Color multiplied with albedo texture, 0.0 -> 1.0
        float roughnessValue{ DefaultRoughness }; // Used if there is no roughness map, 0.0 -> 1.0
        float metalnessValue{ DefaultMetalness }; // Used if there is no metalness map, 0.0 -> 1.0

        ShaderHandle shader{};
        MaterialFlags flags{};

        bool TestFlag(MaterialFlags flag) const;
        void AddFlag(MaterialFlags flag);
        void RemoveFlag(MaterialFlags flag);
    };
}
