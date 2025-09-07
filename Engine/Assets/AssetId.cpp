#include "Engine/Assets/AssetId.h"

#include "Core/IO/FileIO.h"

namespace TombForge
{
    std::string AssetBase::GetFileName() const
    {
        return FileIO::GetFileName(name, false);
    }

    void AssetBase::SetFileName(const std::string& filename)
    {
        std::string basePath = FileIO::GetBasePath(name);
        name = basePath + FileIO::Separator + filename + name.substr(name.find('.'));
    }
}
