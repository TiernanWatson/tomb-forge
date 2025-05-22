#include "Material.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include "../Debug.h"

namespace TombForge
{
    bool Material::SaveJson() const
    {
        const std::string& filePath = name;

        std::ofstream outFile(filePath);

        if (outFile.is_open())
        {
            nlohmann::json json;
            json["diffuse"] = diffuse ? diffuse->name : "";
            json["normal"] = normal ? normal->name : "";
            json["baseColor"] = { baseColor.r, baseColor.g, baseColor.b, baseColor.a };
            json["isTransparent"] = TestFlag(MATERIAL_FLAG_TRANSPARENT);

            outFile << json.dump(4);

            outFile.flush();
            outFile.close();

            return true;
        }

        LOG_ERROR("Could not save material to %s", filePath);
        return false;
    }

    bool Material::TestFlag(MaterialFlags flag) const
    {
        return (flags & flag) == flag;
    }

    void Material::AddFlag(MaterialFlags flag)
    {
        flags = static_cast<MaterialFlags>(flags | flag);
    }

    void Material::RemoveFlag(MaterialFlags flag)
    {
        flags = static_cast<MaterialFlags>(flags & ~flag);
    }
}

