#pragma once

#include <glm/vec4.hpp>

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include "Texture.h"
#include "Graphics.h"

namespace TombForge
{
	enum MaterialFlags : uint8_t
	{
		MATERIAL_FLAG_NONE = 0,
		MATERIAL_FLAG_DIFFUSE = 1,
		MATERIAL_FLAG_NORMAL = 2,
		MATERIAL_FLAG_TRANSPARENT = 4,
		MATERIAL_FLAG_SKINNED = 8,

		MATERIAL_FLAG_DIFFUSE_NORMAL = MATERIAL_FLAG_DIFFUSE | MATERIAL_FLAG_NORMAL
	};

	struct Material
	{
		std::string name{};

		std::shared_ptr<Texture> diffuse{};

		std::shared_ptr<Texture> normal{};

		glm::vec4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f }; // Percentage range

		float roughness{};

		ShaderHandle shader{};

		MaterialFlags flags{};

		bool SaveJson() const;

		bool TestFlag(MaterialFlags flag) const;

		void AddFlag(MaterialFlags flag);

		void RemoveFlag(MaterialFlags flag);
	};
}
