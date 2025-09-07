#include "Engine/ProjectSettings.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Core/Debug.h"

namespace TombForge
{
    void ProjectSettings::LoadJson(const std::string& path)
    {
        std::ifstream in(path);

        if (in.is_open())
        {
            nlohmann::json json;
            in >> json;

            if (json.contains("name"))
            {
                name = json["name"].get<std::string>();
            }

            if (json.contains("laraPath"))
            {
                laraPath = json["laraPath"].get<AssetId>();
            }
            else
            {
                laraPath = InvalidAssetId;
            }

            if (json.contains("defaultLevelPath"))
            {
                defaultLevelPath = json["defaultLevelPath"].get<AssetId>();
            }
            else
            {
                defaultLevelPath = InvalidAssetId;
            }

            in.close();
        }
        else
        {
            LOG_ERROR("Could not load project settings from %s", path.c_str());
        }
    }

    void ProjectSettings::SaveJson(const std::string& path) const
    {
        std::ofstream out(path);

        if (out.is_open())
        {
            nlohmann::json json;
            json["name"] = name;
            json["laraPath"] = IsValidAssetId(laraPath) ? laraPath : InvalidAssetId;
            json["defaultLevelPath"] = IsValidAssetId(defaultLevelPath) ? defaultLevelPath : InvalidAssetId;
            out << json.dump(4);
            out.flush();
            out.close();
        }
        else
        {
            LOG_ERROR("Could not save project settings to %s", path.c_str());
        }
    }
}
