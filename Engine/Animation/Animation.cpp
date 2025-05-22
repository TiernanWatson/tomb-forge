#include "Animation.h"

#include <fstream>

namespace TombForge
{
    bool Animation::Load()
    {
        const std::string& filePath = name;

        std::ifstream inFile(filePath, std::ios::binary);

        if (inFile.is_open())
        {
            size_t numOfKeys{};
            inFile.read((char*)&numOfKeys, sizeof(size_t));

            keys.clear();
            keys.resize(numOfKeys);

            for (size_t b = 0; b < numOfKeys; b++)
            {
                size_t numPositions{};
                inFile.read((char*)&numPositions, sizeof(size_t));

                keys[b].positions.resize(numPositions);
                inFile.read((char*)keys[b].positions.data(), sizeof(PositionKey) * numPositions);

                size_t numRotations{};
                inFile.read((char*)&numRotations, sizeof(size_t));

                keys[b].rotations.resize(numRotations);
                inFile.read((char*)keys[b].rotations.data(), sizeof(RotationKey) * numRotations);

                size_t numScales{};
                inFile.read((char*)&numScales, sizeof(size_t));

                keys[b].scales.resize(numScales);
                inFile.read((char*)keys[b].scales.data(), sizeof(ScaleKey) * numScales);
            }

            size_t numEvents{};
            inFile.read((char*)&numEvents, sizeof(size_t));

            if (numEvents > 0)
            {
                events.resize(numEvents);
                inFile.read((char*)events.data(), sizeof(EventKey) * numEvents);
            }

            inFile.read((char*)&hasRootMotion, sizeof(bool));
            inFile.read((char*)&length, sizeof(float));
            inFile.read((char*)&framerate, sizeof(float));

            inFile.close();

            return true;
        }

        return false;
    }

    bool Animation::SaveBinary() const
    {
        const std::string& filePath = name;

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            const size_t numOfKeys = keys.size();
            outFile.write((const char*)&numOfKeys, sizeof(size_t));

            for (size_t b = 0; b < keys.size(); b++)
            {
                const size_t numPositions = keys[b].positions.size();
                outFile.write((const char*)&numPositions, sizeof(size_t));
                outFile.write((const char*)keys[b].positions.data(), sizeof(PositionKey) * numPositions);

                const size_t numRotations = keys[b].rotations.size();
                outFile.write((const char*)&numRotations, sizeof(size_t));
                outFile.write((const char*)keys[b].rotations.data(), sizeof(RotationKey) * numRotations);

                const size_t numScales = keys[b].scales.size();
                outFile.write((const char*)&numScales, sizeof(size_t));
                outFile.write((const char*)keys[b].scales.data(), sizeof(ScaleKey) * numScales);
            }

            const size_t numEvents = events.size();
            outFile.write((const char*)&numEvents, sizeof(size_t));
            if (numEvents > 0)
            {
                outFile.write((const char*)events.data(), sizeof(EventKey) * numEvents);
            }

            outFile.write((const char*)&hasRootMotion, sizeof(bool));
            outFile.write((const char*)&length, sizeof(float));
            outFile.write((const char*)&framerate, sizeof(float));

            outFile.flush();
            outFile.close();

            return true;
        }

        return false;
    }

    std::string AnimEventToString(AnimEvent event)
    {
        switch (event)
        {
        case ANIM_EVENT_GENERIC:
            return "Generic Event";
        case ANIM_EVENT_STATE_TRANSITION:
            return "State Transition";
        case ANIM_EVENT_LEFT_FOOT:
            return "Left Foot";
        case ANIM_EVENT_RIGHT_FOOT:
            return "Right Foot";
        case ANIM_EVENT_LEFT_HAND:
            return "Left Hand";
        case ANIM_EVENT_RIGHT_HAND:
            return "Right Hand";
        case ANIM_EVENT_GROUND_CONTACT:
            return "Ground Contact";
        case ANIM_EVENT_LEFT_GROUND:
            return "Left Ground";
        case ANIM_EVENT_ROOT_MOVE_ON:
            return "Root Motion On";
        case ANIM_EVENT_ROOT_MOVE_OFF:
            return "Root Motion Off";
        case ANIM_EVENT_COLLISION_ON:
            return "Collision On";
        case ANIM_EVENT_COLLISION_OFF:
            return "Collision Off";
        default:
            return std::string("No name defined");
        }
    }
}
