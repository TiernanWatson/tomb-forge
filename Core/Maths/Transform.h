#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace TombForge
{
	/// <summary>
	/// Represents a scale, rotation and position to be applied in that order
	/// </summary>
	struct Transform
	{
		glm::quat rotation{};

		glm::vec3 position{};

		glm::vec3 scale{ 1.0f };

		glm::mat4 AsMatrix() const;

		glm::vec3 EulerRotation() const;

		glm::vec3 ForwardVector() const;

		void SetEulers(glm::vec3 eulerRotations);
		void SetEulers(float x, float y, float z);

		Transform operator*(const Transform& t2);
	};
}

