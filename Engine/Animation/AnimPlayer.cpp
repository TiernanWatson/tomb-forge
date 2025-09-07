#include "Engine/Animation/AnimPlayer.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>

#include "Core/Debug.h"

namespace TombForge
{
    void AnimPlayer::Play(std::shared_ptr<const Animation> animation, bool loop, float targetFrame)
    {
        m_currentAnim.Clear();
        m_currentAnim.clip = animation;
        m_currentAnim.shouldLoop = loop;
        m_currentAnim.currentFrame = targetFrame;
        m_currentAnim.previousRootPosition = GetPosition(animation->keys[0].positions, m_currentAnim.currentFrame, m_defaultPositions[0], loop, true);
        m_currentAnim.previousRootRotation = GetRotation(animation->keys[0].rotations, m_currentAnim.currentFrame, m_defaultRotations[0]);
        m_currentAnim.previousFrame = m_currentAnim.currentFrame;

        m_isBlending = false;
        m_wasInterrupted = false;
    }

    void AnimPlayer::BlendTo(std::shared_ptr<const Animation> animation, float frames, bool loop, float targetFrame)
    {
        if (m_isBlending)
        {
            // When a blend is interrupted, to avoid snapping, we blend from the current pose
            m_wasInterrupted = true;
            m_blendPose = m_finalPose;
        }
        else
        {
            m_previousAnim = m_currentAnim;
        }

        m_currentAnim.Clear();
        m_currentAnim.clip = animation;
        m_currentAnim.shouldLoop = loop;
        m_currentAnim.currentFrame = targetFrame;
        m_currentAnim.previousRootPosition = GetPosition(animation->keys[0].positions, targetFrame, m_defaultPositions[0], loop);
        m_currentAnim.previousRootRotation = GetRotation(animation->keys[0].rotations, targetFrame, m_defaultRotations[0]);
        m_currentAnim.previousFrame = targetFrame;

        m_isBlending = true;
        m_blendStart = m_currentAnim.currentFrame;
        m_blendTime = frames;
    }

    void AnimPlayer::Process(float deltaTime)
    {
        if (!IsValid() || !m_currentAnim.clip)
        {
            return;
        }

        const bool extractRootMovement = m_rootMotionMode != RootMotionMode::Off;
        const bool extractRootRotation = m_rootMotionMode == RootMotionMode::On;

        for (size_t boneIndex = 0; boneIndex < m_currentAnim.clip->keys.size(); boneIndex++)
        {
            const BoneKeys& keys = m_currentAnim.clip->keys[boneIndex];
            Transform& finalPose = m_finalPose.bones[boneIndex];

            finalPose.position = GetPosition(keys.positions, m_currentAnim.currentFrame, m_defaultPositions[boneIndex], m_currentAnim.shouldLoop, boneIndex == 0);
            if (boneIndex == 0 && extractRootMovement)
            {
                m_rootDelta = m_currentAnim.CalculateRootDelta(finalPose.position);
                finalPose.position = m_defaultPositions[boneIndex];
            }

            finalPose.rotation = GetRotation(keys.rotations, m_currentAnim.currentFrame, m_defaultRotations[boneIndex]);
            if (boneIndex == 0 && extractRootRotation)
            {
                m_rootRotDelta = m_currentAnim.CalculateRootRotDelta(finalPose.rotation);
                finalPose.rotation = m_defaultRotations[boneIndex];
            }

            finalPose.scale = GetScale(keys.scales, m_currentAnim.currentFrame, m_defaultScales[boneIndex]);

            if (m_isBlending)
            {
                const BoneKeys& previousKeys = m_previousAnim.clip->keys[boneIndex];
                const float blendDelta = (m_currentAnim.currentFrame - m_blendStart) / m_blendTime;
                Transform& blendPose = m_blendPose.bones[boneIndex];

                if (!m_wasInterrupted)
                {
                    // When interrupted, we use the last computed pose staticly, so we can have responsiveness and visual fidelity
                    blendPose.position = GetPosition(previousKeys.positions, m_previousAnim.currentFrame, m_defaultPositions[boneIndex], m_previousAnim.shouldLoop, boneIndex == 0);
                    blendPose.rotation = GetRotation(previousKeys.rotations, m_previousAnim.currentFrame, m_defaultRotations[boneIndex]);
                    blendPose.scale = GetScale(previousKeys.scales, m_previousAnim.currentFrame, m_defaultScales[boneIndex]);
                }

                if (boneIndex == 0 && extractRootMovement)
                {
                    const glm::vec3 rootDeltaPrevious = m_wasInterrupted ? glm::vec3{} : m_previousAnim.CalculateRootDelta(blendPose.position);
                    m_rootDelta = glm::mix(rootDeltaPrevious, m_rootDelta, blendDelta);
                }
                else
                {
                    finalPose.position = glm::mix(blendPose.position, finalPose.position, blendDelta);
                }

                if (boneIndex == 0 && extractRootRotation)
                {
                    const glm::quat rootRotDeltaPrevious = m_wasInterrupted ? glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f } : m_previousAnim.CalculateRootRotDelta(blendPose.rotation);
                    m_rootRotDelta = glm::slerp(rootRotDeltaPrevious, m_rootRotDelta, blendDelta);
                }
                else
                {
                    finalPose.rotation = glm::slerp(blendPose.rotation, finalPose.rotation, blendDelta);
                }

                finalPose.scale = glm::mix(blendPose.scale, finalPose.scale, blendDelta);
            }

            const glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.0f }, finalPose.scale);
            const glm::mat4 rotationMatrix = glm::toMat4(glm::normalize(finalPose.rotation));
            const glm::mat4 translateMatrix = glm::translate(glm::mat4{ 1.0f }, finalPose.position);
            
            const glm::mat4 boneMatrix = translateMatrix * rotationMatrix * scaleMatrix;
            if (boneIndex == 0)
            {
                m_finalMatrices[boneIndex] = boneMatrix;
            }
            else
            {
                const uint8_t parent = m_skeleton->bones[boneIndex].parent;
                ASSERT(parent < boneIndex, "The parent index should not be greater than the bone index");
                m_finalMatrices[boneIndex] = m_finalMatrices[parent] * boneMatrix;
            }
        }

        // Don't do this above as the bone offset would affect the matrices incorrectly
        for (size_t b = 0; b < m_finalMatrices.size(); b++)
        {
            m_finalMatrices[b] = m_finalMatrices[b] * m_skeleton->bones[b].offset;
        }

        TriggerEvents(m_currentAnim.clip->events, m_currentAnim.currentFrame);

        m_currentAnim.AdvanceFrame(deltaTime);

        if (m_isBlending)
        {
            if (m_currentAnim.currentFrame - m_blendStart > m_blendTime)
            {
                m_isBlending = false;
                m_wasInterrupted = false;
                m_previousAnim.Clear();
            }
            else
            {
                m_previousAnim.AdvanceFrame(deltaTime);
            }
        }
    }

    void AnimPlayer::SetSkeleton(std::shared_ptr<const Skeleton> skeleton)
    {
        const size_t boneCount = skeleton->bones.size();

        m_skeleton = skeleton;
        m_finalMatrices.resize(boneCount);
        m_defaultPositions.resize(boneCount);
        m_defaultRotations.resize(boneCount);
        m_defaultScales.resize(boneCount);
        m_finalPose.bones.resize(boneCount);
        m_blendPose.bones.resize(boneCount);

        for (size_t boneIndex = 0; boneIndex < boneCount; boneIndex++)
        {
            // Used for fallbacks if a bone is not keyed
            glm::vec3 defaultSkew{}; // ignore
            glm::vec4 defaultPers{}; // ignore

            glm::decompose(
                m_skeleton->bones[boneIndex].transform,
                m_defaultScales[boneIndex],
                m_defaultRotations[boneIndex],
                m_defaultPositions[boneIndex],
                defaultSkew,
                defaultPers);

            if (boneIndex == 0)
            {
                m_finalMatrices[boneIndex] = m_skeleton->bones[boneIndex].transform;
            }
            else
            {
                const uint8_t parent = m_skeleton->bones[boneIndex].parent;
                ASSERT(parent < boneIndex, "The parent index should not be greater than the bone index");
                m_finalMatrices[boneIndex] = m_finalMatrices[parent] * m_skeleton->bones[boneIndex].transform;
            }
        }

        // Don't do this above as the calculations depend on each other
        for (size_t b = 0; b < m_finalMatrices.size(); b++)
        {
            m_finalMatrices[b] = m_finalMatrices[b] * m_skeleton->bones[b].offset;
        }
    }

    glm::vec3 AnimPlayer::GetPosition(const std::vector<PositionKey>& positions, float frame, glm::vec3 fallback, bool looping, bool isRoot) const
    {
        if (positions.empty())
        {
            return fallback;
        }

        if (positions.size() == 1)
        {
            return positions[0].value;
        }

        for (size_t i = 0; i < positions.size() - 1; i++)
        {
            if (positions[i + 1].time > frame)
            {
                const float interp = (frame - positions[i].time) / (positions[i + 1].time - positions[i].time);
                return glm::mix(positions[i].value, positions[i + 1].value, interp);
            }
        }

        // We end up here if we are beyond the last keyframe
        if (looping)
        {
            // Blend between last and first key
            const float interp = frame - positions[positions.size() - 1].time;
            // With the root we want movement to keep going, not snap back
            const glm::vec3 targetPosition = isRoot ? (positions[0].value + positions[positions.size() - 1].value) : positions[0].value;
            return glm::mix(positions[positions.size() - 1].value, targetPosition, interp);
        }
        else
        {
            return positions[positions.size() - 1].value;
        }
    }

    glm::vec3 AnimPlayer::GetScale(const std::vector<ScaleKey>& scales, float frame, glm::vec3 fallback) const
    {
        if (scales.empty())
        {
            return fallback;
        }

        if (scales.size() == 1)
        {
            return scales[0].value;
        }

        for (size_t i = 0; i < scales.size() - 1; i++)
        {
            if (scales[i + 1].time > frame)
            {
                float interp = (frame - scales[i].time) / (scales[i + 1].time - scales[i].time);
                return glm::mix(scales[i].value, scales[i + 1].value, interp);
            }
        }

        return scales[scales.size() - 1].value;
    }

    glm::quat AnimPlayer::GetRotation(const std::vector<RotationKey>& rotations, float frame, glm::quat fallback) const
    {
        if (rotations.empty())
        {
            return fallback;
        }

        if (rotations.size() == 1)
        {
            return rotations[0].value;
        }

        for (size_t i = 0; i < rotations.size() - 1; i++)
        {
            if (rotations[i + 1].time > frame)
            {
                float interp = (frame - rotations[i].time) / (rotations[i + 1].time - rotations[i].time);
                return glm::slerp(rotations[i].value, rotations[i + 1].value, interp);
            }
        }

        return rotations[rotations.size() - 1].value;
    }

    void AnimPlayer::TriggerEvents(const std::vector<EventKey>& events, float frame)
    {
        if (!m_eventCallback || events.size() == 0)
        {
            return;
        }

        if (events.size() == 1)
        {
            if (frame < events[0].time)
            {
                m_lastEvent = 0;
            }

            if (m_lastEvent == 0 && frame > events[0].time)
            {
                m_lastEvent = 1;
                m_eventCallback((AnimEvent)events[0].value);
            }
        }
        else
        {
            const size_t nextEvent = fmodf(m_lastEvent + 1, events.size());
            if (frame > events[nextEvent].time)
            {
                m_lastEvent = nextEvent;
                m_eventCallback((AnimEvent)events[nextEvent].value);
            }
        }
    }

    glm::vec3 AnimPlayer::AnimPlaybackInfo::CalculateRootDelta(glm::vec3 newPosition)
    {
        const auto& keys = clip->keys[0].positions;

        glm::vec3 result = newPosition - previousRootPosition;
        if (previousFrame > currentFrame && keys.size() > 0)
        {
            // Stop root looping back, add on last position
            result += keys[keys.size() - 1].value;
        }
        previousRootPosition = newPosition;

        return result;
    }

    glm::quat AnimPlayer::AnimPlaybackInfo::CalculateRootRotDelta(const glm::quat& newRot)
    {
        const auto& keys = clip->keys[0].rotations;

        const glm::vec3 original = previousRootRotation * glm::vec3{ 0.0f, 0.0f, -1.0f };
        const glm::vec3 newF = newRot * glm::vec3{ 0.0f, 0.0f, -1.0f };

        glm::quat result = glm::rotation(original, newF);
        if (previousFrame > currentFrame && keys.size() > 0)
        {
            // Stop root looping back, add on last position
            result *= keys[keys.size() - 1].value;
        }
        previousRootRotation = newRot;

        return result;
    }

    void AnimPlayer::AnimPlaybackInfo::AdvanceFrame(float deltaTime)
    {
        previousFrame = currentFrame;

        currentFrame += deltaTime * clip->framerate;
        if (shouldLoop)
        {
            // We add one to account for if the loop relies on the blend 
            // between last and first frame (i.e. first frame =/= last frame)
            currentFrame = fmodf(currentFrame, clip->length + 1);
        }

        if (previousFrame > currentFrame)
        {
            loopCount++;
        }
    }

    void AnimPlayer::AnimPlaybackInfo::Clear()
    {
        clip = nullptr;
        currentFrame = 0.0f;
        previousRootPosition = {};
        previousRootRotation = { 1.0f, 0.0f, 0.0f, 0.0f };
        previousFrame = 0.0f;
        loopCount = 0;
        shouldLoop = false;
    }
}
