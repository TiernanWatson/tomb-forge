#version 460 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;

uniform vec4 color;

void main()
{
	FragColor = color;
}
