#include "AnimPlayer.h"

#include <cmath>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "../../Core/Debug.h"

namespace TombForge
{
	void AnimPlayer::Play(std::shared_ptr<const Animation> animPlayer, bool loop)
	{
		m_currentAnim.animPlayer = animPlayer;
		m_currentAnim.previousRootPosition = glm::vec3{};
		m_currentAnim.currentFrame = 0.0f;
		m_currentAnim.loopCount = 0;
		m_currentAnim.shouldLoop = loop;

		m_isBlending = false;

		Process(0.0f);
	}

	void AnimPlayer::BlendTo(std::shared_ptr<const Animation> animPlayer, float frames, bool loop)
	{
		if (m_isBlending)
		{
			//m_currentAnim = m_targetAnim;
		}

		m_targetAnim.animPlayer = animPlayer;
		m_targetAnim.currentFrame = 0.0f;
		m_targetAnim.previousRootPosition = glm::vec3{};
		m_targetAnim.loopCount = 0;
		m_targetAnim.shouldLoop = loop;

		m_isBlending = true;
		m_blendTime = frames;
	}

	void AnimPlayer::Process(float deltaTime)
	{
		if (!IsValid())
		{
			return;
		}

		m_currentAnim.AdvanceFrame(deltaTime);

		if (m_isBlending)
		{
			m_targetAnim.AdvanceFrame(deltaTime);

			if (m_targetAnim.currentFrame > m_blendTime)
			{
				m_isBlending = false;
				m_currentAnim = m_targetAnim;
				m_targetAnim.Clear();
			}
		}

		const bool extractRootMovement = m_rootMotionMode != RootMotionMode::Off;
		const bool extractRootRotation = m_rootMotionMode == RootMotionMode::On;

		for (size_t boneIndex = 0; boneIndex < m_currentAnim.animPlayer->keys.size(); boneIndex++)
		{
			const BoneKeys& keys = m_currentAnim.animPlayer->keys[boneIndex];

			// Position

			glm::vec3 position = GetPosition(keys.positions, m_currentAnim.currentFrame, m_defaultPositions[boneIndex]);
			if (boneIndex == 0 && extractRootMovement)
			{
				m_rootDelta = m_currentAnim.CalculateRootDelta(position);

				// We don't want to animate the root bone if root motion is on
				position = m_defaultPositions[boneIndex];
			}

			// Rotation
			
			glm::quat rotation = GetRotation(keys.rotations, m_currentAnim.currentFrame, m_defaultRotations[boneIndex]);
			if (boneIndex == 0 && extractRootRotation)
			{
				m_rootRotDelta = m_currentAnim.CalculateRootRotDelta(rotation);

				rotation = m_defaultRotations[boneIndex];
			}

			// Scale

			glm::vec3 scale = GetScale(keys.scales, m_currentAnim.currentFrame, m_defaultScales[boneIndex]);

			if (m_isBlending)
			{
				const float blendDelta = m_targetAnim.currentFrame / m_blendTime;

				glm::vec3 targetPosition = GetPosition(m_targetAnim.animPlayer->keys[boneIndex].positions, m_targetAnim.currentFrame, m_defaultPositions[boneIndex]);
				if (boneIndex == 0 && extractRootMovement)
				{
					glm::vec3 rootDeltaTarget = m_targetAnim.CalculateRootDelta(targetPosition);
					m_rootDelta = glm::mix(m_rootDelta, rootDeltaTarget, blendDelta);
				}
				else
				{
					position = glm::mix(position, targetPosition, blendDelta);
				}

				glm::quat targetRotation = GetRotation(m_targetAnim.animPlayer->keys[boneIndex].rotations, m_targetAnim.currentFrame, m_defaultRotations[boneIndex]);
				if (boneIndex == 0 && extractRootRotation)
				{
					glm::quat rootRotDeltaTarget = m_targetAnim.CalculateRootRotDelta(targetRotation);
					m_rootRotDelta = glm::slerp(m_rootRotDelta, rootRotDeltaTarget, blendDelta);
				}
				else
				{
					rotation = glm::slerp(rotation, targetRotation, m_targetAnim.currentFrame / m_blendTime);
				}
			}

			const glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.0f }, scale);
			const glm::mat4 rotationMatrix = glm::toMat4(glm::normalize(rotation));
			const glm::mat4 translateMatrix = glm::translate(glm::mat4{ 1.0f }, position);
			
			const glm::mat4 offsetMatrix = m_skeleton->bones[boneIndex].offset;
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

		// Don't do this above as the calculations depend on each other
		for (size_t b = 0; b < m_finalMatrices.size(); b++)
		{
			m_finalMatrices[b] = m_finalMatrices[b] * m_skeleton->bones[b].offset;
		}

		TriggerEvents(m_currentAnim.animPlayer->events, m_currentAnim.currentFrame);
	}

	void AnimPlayer::SetSkeleton(std::shared_ptr<const Skeleton> skeleton)
	{
		const size_t boneCount = skeleton->bones.size();

		m_skeleton = skeleton;
		m_finalMatrices.resize(boneCount);
		m_defaultPositions.resize(boneCount);
		m_defaultRotations.resize(boneCount);
		m_defaultScales.resize(boneCount);

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

	glm::vec3 AnimPlayer::GetPosition(const std::vector<PositionKey>& positions, float frame, glm::vec3 fallback) const
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
				float interp = (frame - positions[i].time) / (positions[i + 1].time - positions[i].time);
				return glm::mix(positions[i].value, positions[i + 1].value, interp);
			}
		}

		return positions[positions.size() - 1].value;
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
		const auto& keys = animPlayer->keys[0].positions;

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
		const auto& keys = animPlayer->keys[0].rotations;

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

		currentFrame += deltaTime * animPlayer->framerate;
		if (shouldLoop)
		{
			currentFrame = fmodf(currentFrame, animPlayer->length);
		}

		if (previousFrame > currentFrame)
		{
			loopCount++;
		}
	}

	void AnimPlayer::AnimPlaybackInfo::Clear()
	{
		animPlayer = nullptr;
		currentFrame = 0.0f;
		previousRootPosition = {};
		previousFrame = 0.0f;
		loopCount = 0;
		shouldLoop = false;
	}
}
