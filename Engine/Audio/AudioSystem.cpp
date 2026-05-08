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

    void AudioSystem::SetListenerPosition(float x, float y, float z)
    {
        alListener3f(AL_POSITION, x, y, z);
    }

    void AudioSystem::SetListenerDirection(float x, float y, float z, float upX, float upY, float upZ)
    {
        float orientation[6] = { x, y, z, upX, upY, upZ };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    uint16_t AudioSystem::GenerateBuffer(std::shared_ptr<const Sound> sound)
    {
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
            return UINT16_MAX;
        }

        ALuint buffer;
        ALuint source;
        alGenBuffers(1, &buffer);
        alGenSources(1, &source);
        alBufferData(buffer, format, sound->data.data(), static_cast<ALsizei>(sound->data.size() * sizeof(int16_t)), sound->sampleRate);
        m_persistentBuffers.emplace_back(source, buffer);

        return static_cast<uint16_t>(m_persistentBuffers.size() - 1);
    }

    void AudioSystem::PlayBuffer(uint16_t buffer, float x, float y, float z, float volume, bool loop)
    {
        if (buffer >= m_persistentBuffers.size())
        {
            LOG_WARNING("Attempted to play invalid audio buffer: %i", buffer);
            return;
        }
        auto& instance = m_persistentBuffers[buffer];
        alSourcei(instance.source, AL_BUFFER, instance.buffer);
        alSource3f(instance.source, AL_POSITION, x, y, z);
        alSourcef(instance.source, AL_GAIN, volume);
        alSourcei(instance.source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcePlay(instance.source);
    }

    void AudioSystem::DeleteBuffer(uint16_t buffer)
    {
        if (buffer >= m_persistentBuffers.size())
        {
            LOG_WARNING("Attempted to delete invalid audio buffer: %i", buffer);
            return;
        }
        AudioInstance& instance = m_persistentBuffers[buffer];
        alDeleteSources(1, &instance.source);
        alDeleteBuffers(1, &instance.buffer);
        instance.source = 0;
        instance.buffer = 0;
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
