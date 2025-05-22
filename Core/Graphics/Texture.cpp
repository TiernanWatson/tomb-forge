#include "Texture.h"

#include <fstream>

#include "../Debug.h"

namespace TombForge
{
    bool Texture::SaveBinary() const
    {
        if (!IsValidData())
        {
            LOG_ERROR("Tried to save texture %s, but invalid data");
            return false;
        }

        const std::string& filePath = name;

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            outFile.write((const char*)&format, sizeof(TextureFormat));
            outFile.write((const char*)&width, sizeof(uint32_t));
            outFile.write((const char*)&height, sizeof(uint32_t));
            outFile.write((const char*)data.data(), sizeof(ColorByte) * data.size());

            outFile.flush();
            outFile.close();

            return true;
        }

        LOG_ERROR("Could not save texture to %s", filePath);
        return false;
    }
}
