#include "SkeletonRetargetMap.h"

#include "Engine/Animation/Animation.h"
#include "Engine/Animation/Skeleton.h"

namespace TombForge
{
    SkeletonRetargetMap::SkeletonRetargetMap(const Skeleton& source)
    {
        map.resize(source.bones.size(), UINT8_MAX);
        rotations.resize(source.bones.size(), glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f });
        settings.resize(source.bones.size());
    }

    void SkeletonRetargetMap::FillRotations(const Skeleton& source, const Skeleton& target)
    {
        for (size_t sourceBone = 0; sourceBone < source.bones.size(); ++sourceBone)
        {
            const uint8_t targetBone = static_cast<uint8_t>(map[sourceBone]);
            if (targetBone != UINT8_MAX)
            {
                const glm::mat4& sourceTransform = source.bones[sourceBone].transform;
                const glm::mat4& targetTransform = target.bones[targetBone].transform;
                const glm::quat sourceRotation = glm::quat_cast(sourceTransform);
                const glm::quat targetRotation = glm::quat_cast(targetTransform);
                rotations[sourceBone] = targetRotation * glm::inverse(sourceRotation);
            }
        }
    }

    void SkeletonRetargetMap::RetargetAnimation(const Skeleton& source, const Skeleton& target, const Animation& sourceAnim, Animation& animation) const
    {
        for (size_t sourceBone = 0; sourceBone < sourceAnim.keys.size(); sourceBone++)
        {
            const uint8_t targetBone = map[sourceBone];
            if (targetBone == UINT8_MAX)
            {
                continue;
            }

            const auto& sourceKeys = sourceAnim.keys[sourceBone];

            if (settings[sourceBone].usePosition)
            {
                for (size_t t = 0; t < sourceKeys.positions.size(); t++)
                {
                    if (targetBone != UINT8_MAX)
                    {
                        const glm::vec3 retargetedPosition = rotations[sourceBone] * sourceKeys.positions[t].value;
                        animation.keys[targetBone].positions.emplace_back(retargetedPosition, sourceKeys.positions[t].time);
                    }
                }
            }

            if (settings[sourceBone].useRotation)
            {
                for (size_t r = 0; r < sourceKeys.rotations.size(); r++)
                {
                    if (targetBone != UINT8_MAX)
                    {
                        const glm::quat retargetedRotation = rotations[sourceBone] * sourceKeys.rotations[r].value;
                        animation.keys[targetBone].rotations.emplace_back(retargetedRotation, sourceKeys.rotations[r].time);
                    }
                }
            }

            if (settings[sourceBone].useScale)
            {
                for (size_t s = 0; s < sourceKeys.scales.size(); s++)
                {
                    if (targetBone != UINT8_MAX)
                    {
                        animation.keys[targetBone].scales.emplace_back(sourceKeys.scales[s].value, sourceKeys.scales[s].time);
                    }
                }
            }
        }
    }
}
