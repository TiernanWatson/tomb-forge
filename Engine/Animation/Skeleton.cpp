#include "Skeleton.h"

#include <fstream>

namespace TombForge
{
    uint8_t Skeleton::FindBoneId(const std::string& boneName) const
    {
        for (size_t i = 0; i < bones.size(); i++)
        {
            if (boneName == bones[i].name)
            {
                return static_cast<uint8_t>(i);
            }
        }

        return -1;
    }

    bool Skeleton::Load()
    {
        const std::string& filePath = name;

        std::ifstream inFile(filePath, std::ios::binary);

        if (inFile.is_open())
        {
            size_t numOfBones{};
            inFile.read((char*)&numOfBones, sizeof(size_t));

            bones.clear();
            bones.resize(numOfBones);

            for (size_t b = 0; b < numOfBones; b++)
            {
                size_t stringLength{};
                inFile.read((char*)&stringLength, sizeof(size_t));

                bones[b].name.resize(stringLength);
                inFile.read(bones[b].name.data(), sizeof(char) * stringLength);

                inFile.read((char*)&bones[b].offset, sizeof(glm::mat4));
                inFile.read((char*)&bones[b].transform, sizeof(glm::mat4));
                inFile.read((char*)&bones[b].parent, sizeof(uint8_t));
            }

            inFile.close();

            return true;
        }

        return false;
    }

    bool Skeleton::SaveBinary() const
    {
        const std::string& filePath = name;

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            const size_t numOfBones = bones.size();
            outFile.write((const char*)&numOfBones, sizeof(size_t));

            for (size_t b = 0; b < bones.size(); b++)
            {
                const size_t stringLength = bones[b].name.length();
                outFile.write((const char*)&stringLength, sizeof(size_t));
                outFile.write(bones[b].name.c_str(), sizeof(char) * stringLength);

                outFile.write((const char*)&bones[b].offset, sizeof(glm::mat4));
                outFile.write((const char*)&bones[b].transform, sizeof(glm::mat4));
                outFile.write((const char*)&bones[b].parent, sizeof(uint8_t));
            }

            outFile.flush();
            outFile.close();

            return true;
        }

        return false;
    }
}
