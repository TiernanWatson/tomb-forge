#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec2 aUv;
layout (location = 4) in ivec4 aBoneIds; 
layout (location = 5) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

invariant gl_Position;

void main()
{
	vec4 basePosition = vec4(aPosition, 1.0f);
	gl_Position = projection * view * model * basePosition;
	gl_Position.z += 0.001;
}