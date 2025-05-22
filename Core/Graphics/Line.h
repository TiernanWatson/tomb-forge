#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace TombForge
{
	struct LineVertex
	{
		glm::vec3 point{};
		glm::vec4 color{};
	};

	struct Line
	{
		LineVertex v0{};
		LineVertex v1{};
	};
}

