#include "MaterialLoader.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "../../Core/IO/FileIO.h"

namespace TombForge
{
    std::shared_ptr<Material> MaterialLoader::Read(const std::string& name)
    {
        std::ifstream inFile(name);

        if (!inFile.is_open())
        {
            LOG_ERROR("Failed to open %s", name.c_str());
            return nullptr;
        }

        nlohmann::json json = nlohmann::json::parse(inFile);
        
        if (json.size() < 1)
        {
            LOG_WARNING("Empty material %s", name.c_str());
            return nullptr;
        }

        std::shared_ptr<Material> resource = std::make_shared<Material>();
        resource->name = name;

        if (m_textureLoader)
        {
            if (json.contains("diffuse"))
            {
                const std::string diffusePath = json["diffuse"].get<std::string>();
                if (!diffusePath.empty())
                {
                    resource->diffuse = m_textureLoader->Load(diffusePath);
                    resource->AddFlag(MATERIAL_FLAG_DIFFUSE);
                }
            }

            if (json.contains("normal"))
            {
                const std::string normalPath = json["normal"].get<std::string>();
                if (!normalPath.empty())
                {
                    resource->normal = m_textureLoader->Load(normalPath);
                    resource->AddFlag(MATERIAL_FLAG_NORMAL);
                }
            }
        }

        if (json.contains("baseColor"))
        {
            auto baseColor = json["baseColor"];
            resource->baseColor = glm::vec4{ baseColor[0], baseColor[1], baseColor[2], baseColor[3] };
        }

        if (json.contains("isTransparent"))
        {
            bool isTransparent = json["isTransparent"];
            if (isTransparent)
            {
                resource->AddFlag(MATERIAL_FLAG_TRANSPARENT);
            }
        }

        return resource;
    }
}
