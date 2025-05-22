#pragma once

#include "LaraState.h"

#include "glm/glm.hpp"

namespace TombForge
{
	class LocomotionState : public LaraBaseState
	{
	public:
		virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) override;

		virtual void UpdateAnimation(LaraController& lara, float deltaTime) override;

		virtual void PostAnimationUpdate(LaraController& lara, float deltaTime) override;

		virtual void PostPhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics) override;

		virtual LaraState ShouldTransition(LaraController& lara) override;

	private:
		bool FindLedge(glm::vec3 position, glm::vec3 direction, PhysicsInterface& physics);

		static constexpr float DefaultRunSpeed{ 3.5f };
		static constexpr float DefaultWalkSpeed{ 1.355f };
		static constexpr float DefaultTurnRate{ 20.0f };
		static constexpr float DefaultRunThreshold{ 0.75f };
		static constexpr float DefaultWalkThreshold{ 0.1f };

		glm::vec3 m_ledgePosition{};
		glm::vec3 m_ledgeForward{};
		glm::vec3 m_moveInput{};
		glm::vec3 m_desiredVelocity{};

		float m_runSpeed{ DefaultRunSpeed };
		float m_walkSpeed{ DefaultWalkSpeed };
		float m_slerpRate{ DefaultTurnRate };
		float m_walkThreshold{ DefaultWalkThreshold };
		float m_runThreshold{ DefaultRunThreshold };
		float m_targetSpeed{};

		bool m_wantsToJump{};
		bool m_foundLedgeForJump{};
	};
}

