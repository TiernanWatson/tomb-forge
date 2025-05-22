#pragma once

#include <string>

#include <glad/glad.h>

#include "Graphics.h"

namespace TombForge
{
	struct Shader
	{
		std::string vertexSource{};

		std::string fragmentSource{};

		ShaderHandle gpuHandle{};
	};
}

