#include "ProjectSettings.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Debug.h"

namespace TombForge
{
    void ProjectSettings::LoadJson(const std::string& path)
    {
        std::ifstream in(path);

        if (in.is_open())
        {
            nlohmann::json json;
            in >> json;
            name = json["name"].get<std::string>();
            laraPath = json["laraPath"].get<std::string>();
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
            json["laraPath"] = laraPath;
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
