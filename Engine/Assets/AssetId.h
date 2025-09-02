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
        AssetId id{ InvalidAssetId };

        std::string name{};
    };
}
