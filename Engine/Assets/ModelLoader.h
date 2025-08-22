#pragma once

#include "AssetLoader.h"
#include "MaterialLoader.h"
#include "../../Core/Graphics/Model.h"

namespace TombForge
{
    class ModelLoader : public AssetLoader<Model>
    {
    public:
        inline void SetMaterialLoader(std::shared_ptr<MaterialLoader> materialLoader)
        {
            m_materialLoader = materialLoader;
        }

    protected:
        virtual std::shared_ptr<Model> Read(const std::string& name) override;

    private:
        std::shared_ptr<MaterialLoader> m_materialLoader{};
    };
}

