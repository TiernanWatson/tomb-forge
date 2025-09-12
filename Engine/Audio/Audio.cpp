#include "Engine/Audio/Audio.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include <vector>

#include "Core/Debug.h"

namespace TombForge
{
    void InitAudio(AudioContext& ctx)
    {
        ALCdevice* device = alcOpenDevice(nullptr); // Open default device
        if (!device)
        {
            LOG_ERROR("Failed to open audio device");
            return;
        }

        ALCcontext* context = alcCreateContext(device, nullptr);
        if (!context || alcMakeContextCurrent(context) == ALC_FALSE)
        {
            if (context)
            {
                alcDestroyContext(context);
            }
            alcCloseDevice(device);
            LOG_ERROR("Failed to set audio context");
            return;
        }
    }

    void DestroyAudio(AudioContext& ctx)
    {
        if (ctx.context)
        {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(ctx.context);
            ctx.context = nullptr;
        }

        if (ctx.device)
        {
            alcCloseDevice(ctx.device);
            ctx.device = nullptr;
        }
    }
}
