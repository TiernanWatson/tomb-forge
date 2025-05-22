#pragma once

#include "TextureImport.h"
#include <cstdint>
#include <stb_image.h>
#include <tiff.h>
#include <tiffio.h>

namespace TombForge
{
    bool ImportTexture(const std::string& path, Texture& texture)
    {
        int width, height, channelCount;
        unsigned char* loadedImage = stbi_load(path.c_str(), &width, &height, &channelCount, 0);

        if (loadedImage)
        {
            switch (channelCount)
            {
            case 1:
                texture.format = TextureFormat::R;
                break;
            case 3:
                texture.format = TextureFormat::RGB;
                break;
            case 4:
                texture.format = TextureFormat::RGBA;
                break;
            default:
                return false;
            }

            texture.width = static_cast<uint32_t>(width);
            texture.height = static_cast<uint32_t>(height);

            const size_t numberOfBytes = static_cast<size_t>(width) * height * channelCount;
            texture.data.resize(numberOfBytes);
            memcpy(texture.data.data(), loadedImage, numberOfBytes);
            stbi_image_free(loadedImage);

            return true;
        }
        else // Try TIFF loader
        {
            bool imported = false;
            TIFF* img = TIFFOpen(path.c_str(), "r");
            if (img)
            {
                TIFFGetField(img, TIFFTAG_IMAGEWIDTH, &texture.width);
                TIFFGetField(img, TIFFTAG_IMAGELENGTH, &texture.height);
                size_t pixelCount = texture.width * texture.height;
                uint32_t* raster = (uint32_t*)_TIFFmalloc(pixelCount * sizeof(uint32_t));
                if (raster != NULL)
                {
                    if (TIFFReadRGBAImage(img, texture.width, texture.height, raster, 0))
                    {
                        texture.format = TextureFormat::RGBA;
                        texture.data.resize(texture.width * texture.height * 4);
                        for (size_t p = 0; p < pixelCount; p += 1)
                        {
                            texture.data[p * 4] = TIFFGetR(raster[p]);
                            texture.data[p * 4 + 1] = TIFFGetG(raster[p]);
                            texture.data[p * 4 + 2] = TIFFGetB(raster[p]);
                            texture.data[p * 4 + 3] = TIFFGetA(raster[p]);
                        }
                        imported = true;
                    }
                    _TIFFfree(raster);
                }
                TIFFClose(img);
            }
            return imported;
        }

        return false;
    }
}

