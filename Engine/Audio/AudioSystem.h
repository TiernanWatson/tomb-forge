#pragma once

#include <memory>
#include <unordered_map>

#include "Engine/Audio/Audio.h"
#include "Engine/Audio/Sound.h"
#include <cstdint>

namespace TombForge
{
    class AudioSystem
    {
    public:
        AudioSystem();
        ~AudioSystem();

        void SetListenerPosition(float x, float y, float z);
        void SetListenerDirection(float x, float y, float z, float upX, float upY, float upZ);

        uint16_t GenerateBuffer(std::shared_ptr<const Sound> sound);
        void PlayBuffer(uint16_t buffer, float x = 0.0f, float y = 0.0f, float z = 0.0f, float volume = 1.0f, bool loop = false);
        void DeleteBuffer(uint16_t buffer);

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
        std::vector<AudioInstance> m_persistentBuffers{};
    };
}

