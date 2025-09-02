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
}
