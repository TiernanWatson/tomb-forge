#include "ClimbState.h"#

#include "../Input.h"
#include "../Lara.h"
#include "../../../Core/Maths/Maths.h"

namespace TombForge
{
	void ClimbState::Begin(LaraController& lara)
	{
		lara.SetRootMotion(RootMotionMode::On);
		m_wantsClimbUp = false;
	}

	void ClimbState::PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics)
	{
		
	}

	void ClimbState::PreAnimationUpdate(LaraController& lara, float deltaTime)
	{
		m_velocity = 0.0f;

		if (Input::GetKey(GLFW_KEY_D, GLFW_PRESS))
		{
			m_velocity += 1.0f;
		}

		if (Input::GetKey(GLFW_KEY_A, GLFW_PRESS))
		{
			m_velocity += -1.0f;
		}

		if (Input::GetKey(GLFW_KEY_SPACE, GLFW_PRESS))
		{
			m_wantsClimbUp = true;
		}
	}

	void ClimbState::UpdateAnimation(LaraController& lara, float deltaTime)
	{
		switch (lara.CurrentAnim())
		{
		case LARA_ANIM_GRAB_WALL:
		{
			if (lara.AnimTimeLeft() < 1.0f)
			{
				lara.SetAnimation(LARA_ANIM_HANG_LOOP);
			}
			break;
		}
		case LARA_ANIM_HANG_LOOP:
		{
			if (m_velocity > 0.1f)
			{
				lara.SetAnimation(LARA_ANIM_SHIMMY_RIGHT, 0.0f, true);
			}
			else if (m_velocity < -0.1f)
			{
				lara.SetAnimation(LARA_ANIM_SHIMMY_LEFT, 0.0f, true);
			}
			else if (m_wantsClimbUp)
			{
				lara.SetAnimation(LARA_ANIM_CLIMB_UP);
			}
			break;
		}
		case LARA_ANIM_SHIMMY_LEFT:
		case LARA_ANIM_SHIMMY_RIGHT:
		{
			if (Maths::Abs(m_velocity) < 0.1f)
			{
				lara.SetAnimation(LARA_ANIM_HANG_LOOP, 3.0f, true);
			}
			break;
		}
		case LARA_ANIM_CLIMB_UP:
		{
			if (lara.AnimTimeLeft() < 3.0f)
			{
				lara.SetAnimation(LARA_ANIM_IDLE, 3.0f, true);
			}
			break;
		}
		default:
			lara.SetAnimation(LARA_ANIM_HANG_LOOP, 0.0, true);
			break;
		}
	}

	void ClimbState::PostAnimationUpdate(LaraController& lara, float deltaTime)
	{
		if (true || lara.GetRootMotionMode() != RootMotionMode::Off)
		{
			const glm::vec3 rootMove = lara.RootDelta() / deltaTime;
			const glm::vec3 actualVel{ rootMove.x, rootMove.z, -rootMove.y };
			lara.SetVelocity(lara.GetRotation() * actualVel);
		}
	}

	void ClimbState::Exit(LaraController& lara)
	{
	}

	LaraState ClimbState::ShouldTransition(LaraController& lara)
	{
		if (lara.CurrentAnim() == LARA_ANIM_CLIMB_UP)
		{
			// This stops the incorrect shift to air state and allows the capsule to ground itself
			if (lara.AnimTimeLeft() < 5.0f)
			{
				lara.SetCollidesWithWorld(true);
				return LARA_STATE_LOCOMOTION;
			}
		}
		else if (lara.CurrentAnim() == LARA_ANIM_IDLE)
		{
			// Shouldn't end up here
			lara.SetCollidesWithWorld(true);
			return LARA_STATE_LOCOMOTION;
		}

		return LaraBaseState::ShouldTransition(lara);
	}
}
