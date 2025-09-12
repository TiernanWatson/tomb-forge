#include "Engine/Audio/AudioSystem.h"

#include <AL/al.h>

#include "Core/Debug.h"

namespace TombForge
{
    // This is to keep the OpenAL header out of the AudioSystem.h file
    static_assert(sizeof(ALuint) == sizeof(uint32_t), "ALuint size does not match uint32_t size (used in AudioSystem::AudioInstance)");

    AudioSystem::AudioSystem()
    {
        InitAudio(m_audioContext);
    }

    AudioSystem::~AudioSystem()
    {
        DestroyAudio(m_audioContext);
    }

    void AudioSystem::PlaySound(std::shared_ptr<const Sound> sound, float volume, bool loop)
    {
        auto it = m_activeSounds.find(sound->id);
        if (it != m_activeSounds.end())
        {
            alSourceRewind(it->second.source);
            alSourcePlay(it->second.source);
            return;
        }

        ALenum format = AL_NONE;
        if (sound->channels == SOUND_CHANNEL_MONO)
        {
            format = AL_FORMAT_MONO16;
        }
        else if (sound->channels == SOUND_CHANNEL_STEREO)
        {
            format = AL_FORMAT_STEREO16;
        }
        else
        {
            LOG_ERROR("Unsupported number of channels: %i in file: %s", sound->channels, sound->name);
            return;
        }

        ALuint buffer;
        ALuint source;
        alGenBuffers(1, &buffer);
        alGenSources(1, &source);

        alBufferData(buffer, format, sound->data.data(), static_cast<ALsizei>(sound->data.size() * sizeof(int16_t)), sound->sampleRate);

        alSourcei(source, AL_BUFFER, buffer);
        alSourcef(source, AL_GAIN, volume);
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcePlay(source);

        m_activeSounds.emplace(sound->id, AudioInstance{ source, buffer });
    }

    void AudioSystem::StopAllSounds()
    {
        for (auto& [id, instance] : m_activeSounds)
        {
            alSourceStop(instance.source);
            alDeleteSources(1, &instance.source);
            alDeleteBuffers(1, &instance.buffer);
        }
        m_activeSounds.clear();
    }

    void AudioSystem::Update(float dt)
    {
        for (auto it = m_activeSounds.begin(); it != m_activeSounds.end(); )
        {
            ALint state{};
            alGetSourcei(it->second.source, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING)
            {
                alDeleteSources(1, &it->second.source);
                alDeleteBuffers(1, &it->second.buffer);
                it = m_activeSounds.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
