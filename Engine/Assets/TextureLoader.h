#pragma once

#include "AssetLoader.h"

#include "../../Core/Graphics/Texture.h"

namespace TombForge
{
	class TextureLoader : public AssetLoader<Texture>
	{
	protected:
		virtual std::shared_ptr<Texture> Read(const std::string& name) override;
	};
}

