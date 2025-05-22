#include "TextureLoader.h"

#include <fstream>

namespace TombForge
{
    std::shared_ptr<Texture> TextureLoader::Read(const std::string& name)
    {
        std::ifstream inFile(name, std::ios::binary);

        if (inFile.is_open())
        {
            std::shared_ptr<Texture> resource = std::make_shared<Texture>();
            resource->name = name;

            inFile.read((char*)&resource->format, sizeof(TextureFormat));
            inFile.read((char*)&resource->width, sizeof(uint32_t));
            inFile.read((char*)&resource->height, sizeof(uint32_t));

            const size_t dataSize = static_cast<size_t>(resource->format) 
                * resource->width 
                * resource->height;
            resource->data.resize(dataSize);

            inFile.read((char*)resource->data.data(), dataSize);

            inFile.close();

            return resource;
        }

        return nullptr;
    }
}
