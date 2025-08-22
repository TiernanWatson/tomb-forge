#pragma once

#include "AssetLoader.h"
#include "TextureLoader.h"
#include "../../Core/Graphics/Material.h"

namespace TombForge
{
    class MaterialLoader : public AssetLoader<Material>
    {
    public:
        inline void SetTextureLoader(std::shared_ptr<TextureLoader> textureLoader)
        {
            m_textureLoader = textureLoader;
        }

    protected:
        virtual std::shared_ptr<Material> Read(const std::string& name) override;

    private:
        std::shared_ptr<TextureLoader> m_textureLoader{};
    };
}

