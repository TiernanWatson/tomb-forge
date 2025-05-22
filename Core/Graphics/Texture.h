#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <glm/vec4.hpp>

#include "Graphics.h"

namespace TombForge
{
	using ColorByte = unsigned char;

	enum class TextureFormat : uint8_t
	{
		R = 0,
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

	struct Texture
	{
		std::string name{};

		std::vector<ColorByte> data{};

		uint32_t width{};

		uint32_t height{};

		TextureHandle gpuHandle{};

		TextureFormat format{};

		TextureDataType type{};

		TextureFilter filter{ TextureFilter::Trilinear };

		inline bool IsValidData() const
		{
			// Attempts to check that the texture is initialized correctly
			return data.size() == static_cast<size_t>(format) * width * height;
		}

		bool SaveBinary() const;
	};
}
