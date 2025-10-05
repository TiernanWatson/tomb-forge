#pragma once

struct ALCdevice;
struct ALCcontext;

namespace TombForge
{
    struct AudioContext
    {
        ALCdevice* device{};
        ALCcontext* context{};
    };

    void InitAudio(AudioContext& ctx);
    void DestroyAudio(AudioContext& ctx);
}
