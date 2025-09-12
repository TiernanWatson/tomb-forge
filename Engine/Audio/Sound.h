#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Assets/AssetId.h"

namespace TombForge
{
    enum SoundFormat : uint8_t
    {
        SOUND_FORMAT_WAV = 1,
        SOUND_FORMAT_OGG = 2,
    };

    enum SoundChannel : uint8_t
    {
        SOUND_CHANNEL_MONO = 1,
        SOUND_CHANNEL_STEREO = 2,
    };

    struct Sound : public AssetBase
    {
        std::vector<short> data{}; // Raw audio data
        uint64_t numFrames{};
        uint32_t sampleRate{};
        SoundFormat format{};
        SoundChannel channels{};
    };
}
