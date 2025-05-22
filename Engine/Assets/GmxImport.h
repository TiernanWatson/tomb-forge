#pragma once

#include <memory>
#include <string>

#include "../Levels/Level.h"

namespace TombForge
{
	struct GmxImportSettings
	{
		float scale{ 0.0024f };

		bool importGeometry{ true };

		bool importCollision{ true };

		bool makeYUp{ true };

		bool optimize{ true }; // One model per room and join meshes by material
	};

	struct GmxResult
	{
		//std::shared_ptr<Model> geometry{}; // References to materials & textures filled out
		
		std::vector<std::shared_ptr<Model>> geometry{}; // References to materials & textures filled out

		std::vector<BoxCollider> boxColliders{};

		std::vector<MeshCollider> meshColliders{};

		std::vector<PointLight> lights{};
	};

	GmxResult ImportGmx(const std::string& filePath, const GmxImportSettings& settings);
}
