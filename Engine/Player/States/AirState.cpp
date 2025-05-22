#include "AirState.h"

#include <glm/gtx/vector_angle.hpp>

#include "../../Physics/PhysicsInterface.h"
#include "../../Rendering/JoltDebugRenderer.h"
#include "../Core/Debug.h"
#include "../Input.h"
#include "../Lara.h"

namespace TombForge
{
	static const glm::vec4 s_debugRayColor{ 1.0f, 0.0f, 0.0f, 1.0f };
	static const glm::vec4 s_debugFoundColor{ 0.0f, 1.0f, 0.0f, 1.0f };

	void AirState::Begin(LaraController& lara)
	{
		m_isReaching = false;
		m_foundLedge = false;
	}

	void AirState::PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics)
	{
		if (m_isReaching)
		{
			const glm::vec3 forwardOrigin = lara.GetPosition() + glm::vec3{0.0f, 1.9f, 0.0f};
			const glm::vec3 forwardDir = -lara.GetForward();
			const Ray forwardRay{ forwardOrigin, forwardDir };
			HitResult resultForward{};

			DEBUG_RAY(physics, forwardRay, s_debugRayColor);

			if (physics.Raycast(forwardRay, resultForward))
			{
				const glm::vec3 downOrigin = resultForward.point + forwardDir * 0.1f + glm::vec3{ 0.0f, 0.2f, 0.0f };
				const glm::vec3 downDir = glm::vec3{ 0.0f, -0.2f, 0.0f };
				const Ray downRay{ downOrigin, downDir };
				HitResult resultDown{};

				DEBUG_RAY(physics, downRay, s_debugRayColor);

				if (physics.Raycast(downRay, resultDown))
				{
					const float surfaceAngle = angle(glm::vec3{ 0.0f, 1.0f, 0.0f }, resultDown.normal);

					if (surfaceAngle < glm::radians(10.0f))
					{
						const glm::vec3 ledgePoint{ resultForward.point.x, resultDown.point.y, resultForward.point.z };

						m_foundLedge = true;
						m_ledgePosition = ledgePoint + glm::vec3{ 0.0f, -2.1f, 0.0 } + resultForward.normal * 0.125f;
						m_ledgeForward = -resultForward.normal;

						physics.SetPlayerCollidesWorld(false);
					}
				}
			}
		}
	}

	void AirState::PreAnimationUpdate(LaraController& lara, float deltaTime)
	{
		glm::vec3 actualVelocity = lara.GetVelocity();
		actualVelocity.y += -9.8f * deltaTime;

		lara.SetVelocity(actualVelocity);

		if (Input::GetKey(GLFW_KEY_E, GLFW_PRESS))
		{
			m_isReaching = true;

			lara.SetColliderOffset(lara.GetForward() * 0.25f);
		}
	}

	void AirState::UpdateAnimation(LaraController& lara, float deltaTime)
	{
		switch (lara.CurrentAnim())
		{
		case LARA_ANIM_RUN_TO_JUMP_L:
		case LARA_ANIM_RUN_TO_JUMP_R:
		{
			lara.SetAnimation(LARA_ANIM_RUN_JUMP_L, 3.0f);
			break;
		}
		case LARA_ANIM_RUN_JUMP_L:
		{
			if (lara.AnimTimeLeft() < 3.0f)
			{
				lara.SetAnimation(LARA_ANIM_JUMP_TO_FALL, 3.0f);
			}
			else if (m_isReaching)
			{
				lara.SetAnimation(LARA_ANIM_JUMP_TO_REACH, 0.0f);
			}
			break;
		}
		case LARA_ANIM_JUMP_TO_FALL:
		{
			if (lara.AnimTimeLeft() < 3.0f)
			{
				lara.SetAnimation(LARA_ANIM_FALL, 3.0f);
			}
			else if (m_isReaching)
			{
				lara.SetAnimation(LARA_ANIM_JUMP_TO_REACH, 0.0f);
			}
			break;
		}
		case LARA_ANIM_JUMP_TO_REACH:
		{
			if (lara.AnimTimeLeft() < 1.0f)
			{
				lara.SetAnimation(LARA_ANIM_REACH);
			}
			break;
		}
		case LARA_ANIM_GRAB_LEDGE:
		case LARA_ANIM_REACH:
		case LARA_ANIM_FALL:
			break;
		default:
		{
			lara.SetAnimation(LARA_ANIM_FALL);
			break;
		}
		}
	}

	void AirState::Exit(LaraController& lara)
	{
		lara.SetColliderOffset({ 0.0f, 0.0f, 0.0f });
	}

	LaraState AirState::ShouldTransition(LaraController& lara)
	{
		if (m_foundLedge)
		{
			lara.SetAnimation(LARA_ANIM_GRAB_WALL);
			lara.SetVelocity({});
			lara.SetColliderOffset({});
			lara.SetPosition(m_ledgePosition);
			return LARA_STATE_CLIMB;
		}

		if (lara.GetVelocity().y < 0.0f && lara.IsGrounded())
		{
			return LARA_STATE_LOCOMOTION;
		}

		return LaraBaseState::ShouldTransition(lara);
	}
}
