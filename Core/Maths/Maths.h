#pragma once

#include <glm/vec3.hpp>

namespace TombForge::Maths
{
	inline float Clamp(float value, float min, float max)
	{
		if (value < min)
		{
			return min;
		}
		else if (value > max)
		{
			return max;
		}
		return value;
	}

	inline float Abs(float value)
	{
		return value > 0.0f ? value : -value;
	}

	inline float Max(float a, float b)
	{
		return a > b ? a : b;
	}

	inline float Min(float a, float b)
	{
		return a < b ? a : b;
	}
}

