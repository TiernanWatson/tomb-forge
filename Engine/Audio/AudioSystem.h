#pragma once

#include <memory>
#include <unordered_map>

#include "Engine/Audio/Audio.h"
#include "Engine/Audio/Sound.h"

namespace TombForge
{
    class AudioSystem
    {
    public:
        AudioSystem();
        ~AudioSystem();

        void PlaySound(std::shared_ptr<const Sound> sound, float volume = 1.0f, bool loop = false);
        void StopAllSounds();

        void Update(float dt);

    private:
        struct AudioInstance
        {
            uint32_t source{};
            uint32_t buffer{};
        };

        AudioContext m_audioContext{};
        std::unordered_map<AssetId, AudioInstance> m_activeSounds{};
    };
}

