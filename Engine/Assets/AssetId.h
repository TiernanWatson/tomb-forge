#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace TombForge
{
    using AssetId = size_t;

    constexpr AssetId InvalidAssetId{ std::numeric_limits<size_t>().max() };

    inline bool IsValidAssetId(AssetId id)
    {
        return id != InvalidAssetId;
    }

    struct AssetBase
    {
        AssetId id{ InvalidAssetId }; // Internal use only, do not set manually
        std::string name{}; // Internal use only, do not set manually
        bool isDirty{}; // Has been modified since last save

        std::string GetFileName() const; // Get the file name without the path or extension
        void SetFileName(const std::string& filename); // Set the name from a file name, without path or extension
        inline bool IsValid() const { return IsValidAssetId(id); }
    };
}
