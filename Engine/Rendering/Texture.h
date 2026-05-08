#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Assets/AssetId.h"
#include "Engine/Rendering/Graphics.h"

namespace TombForge
{
    using ColorByte = unsigned char;

    enum class TextureFormat : uint8_t
    {
        R = 1,
        RGB = 3,
        RGBA = 4
    };

    enum class TextureDataType : uint8_t
    {
        Byte = 0,
        Float
    };

    enum class TextureFilter : uint8_t
    {
        Nearest = 0,
        Bilinear,
        Trilinear
    };

    struct Texture : public AssetBase
    {
        std::vector<ColorByte> data{};
        uint32_t width{};
        uint32_t height{};
        TextureHandle gpuHandle{};
        TextureFormat format{};
        TextureDataType type{};
        TextureFilter filter{ TextureFilter::Trilinear };
        bool sRGB{ true }; // Generally true for albedo textures, false for normals

        inline bool IsValidData() const
        {
            // Attempts to check that the texture is initialized correctly
            return data.size() == static_cast<size_t>(format) * width * height;
        }
    };

    inline bool TextureValidOnGPU(const Texture* texture)
    {
        return texture && texture->gpuHandle.IsValid();
    }

    inline bool ValidTextureNotOnGPU(const Texture* texture)
    {
        return texture && !texture->gpuHandle.IsValid() && texture->IsValidData();
    }
}
