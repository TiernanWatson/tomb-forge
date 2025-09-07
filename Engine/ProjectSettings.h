#pragma once

#include <string>

#include "Engine/Assets/AssetId.h"

namespace TombForge
{
    /// <summary>
    /// Used to store details about a project for use in the editor
    /// </summary>
    struct ProjectSettings
    {
        std::string name{}; // Project name
        std::string directory{}; // Absolute file path when loaded

        AssetId laraPath{ InvalidAssetId }; // Relative path to Lara model
        AssetId defaultLevelPath{ InvalidAssetId }; // Relative path to default level

        void LoadJson(const std::string& path);
        void SaveJson(const std::string& path) const;
    };
}
