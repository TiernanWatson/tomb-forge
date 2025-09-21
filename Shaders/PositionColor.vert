#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;

out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	Color = aColor;

	vec4 basePosition = vec4(aPosition, 1.0f);
	gl_Position = projection * view * model * basePosition;
}