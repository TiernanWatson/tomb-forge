#include "LaraController.h"

#include "Lara.h"
#include "../Physics/Physics.h"
#include "../Physics/PhysicsInterface.h"

namespace TombForge
{
	LaraController::LaraController(Lara* lara, PhysicsInterface* physics)
		: m_lara{ lara }, m_physics{ physics }
	{
	}

	void LaraController::SetAnimation(LaraAnim id, float fadeTime, bool loop)
	{
		ASSERT(m_lara->animations[id], "Null animation for Lara: %i", id);

		if (fadeTime == 0.0f)
		{
			m_lara->animPlayer.Play(m_lara->animations[id], loop);
		}
		else
		{
			m_lara->animPlayer.BlendTo(m_lara->animations[id], fadeTime, loop);
		}

		m_lara->animIndex = id;
	}

	void LaraController::SetRootMotion(RootMotionMode mode)
	{
		m_lara->animPlayer.SetRootMotionMode(mode);
	}

	float LaraController::AnimTimeLeft() const
	{
		return m_lara->animPlayer.TimeLeft();
	}

	RootMotionMode LaraController::GetRootMotionMode() const
	{
		return m_lara->animPlayer.GetRootMotionMode();
	}

	glm::vec3 LaraController::RootDelta() const
	{
		return m_lara->animPlayer.RootDelta();
	}

	glm::quat LaraController::RootRotDelta() const
	{
		return m_lara->animPlayer.RootRotDelta();
	}

	LaraAnim LaraController::CurrentAnim() const
	{
		return m_lara->animIndex;
	}

	void LaraController::SetVelocity(const glm::vec3& velocity)
	{
		//m_lara->physics->SetLinearVelocity(GlmVec3ToJph(velocity));
		m_lara->actualVelocity = velocity;
	}

	void LaraController::SetRotation(const glm::quat& rotation)
	{
		m_lara->transform.rotation = rotation;
	}

	void LaraController::SetRotation(const glm::vec3& eulers)
	{
		m_lara->transform.SetEulers(eulers);
	}

	void LaraController::SetPosition(const glm::vec3& position)
	{
		m_lara->transform.position = position;
		m_lara->physics->SetPosition(GlmVec3ToJph(position));
	}

	void LaraController::SetColliderOffset(const glm::vec3& offset)
	{
		m_lara->physics->SetShapeOffset(GlmVec3ToJph(offset));
	}

	void LaraController::SetCollidesWithWorld(bool value)
	{
		m_physics->SetPlayerCollidesWorld(value);
	}

	glm::vec3 LaraController::GetVelocity() const
	{
		return m_lara->actualVelocity;
	}

	glm::quat LaraController::GetRotation() const
	{
		return m_lara->transform.rotation;
	}

	glm::vec3 LaraController::GetPosition() const
	{
		return m_lara->transform.position;
	}

	glm::vec3 LaraController::GetForward() const
	{
		return m_lara->transform.ForwardVector();
	}

	bool LaraController::IsGrounded() const
	{
		return m_lara->physics->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
	}

	float LaraController::CameraYaw() const
	{
		return m_lara->cameraYaw;
	}

	float LaraController::CameraPitch() const
	{
		return m_lara->cameraPitch;
	}
}
