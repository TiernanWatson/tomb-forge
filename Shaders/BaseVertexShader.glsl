#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aBitangent;
layout (location = 4) in vec4 aColor;
layout (location = 5) in vec2 aUv;
layout (location = 6) in ivec4 aBoneIds; 
layout (location = 7) in vec4 aWeights;

out vec3 Normal;
out vec2 TexCoords;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 basePosition = vec4(aPosition, 1.0f);

	mat3 normalMatrix = transpose(inverse(mat3(model)));

	TexCoords = aUv;
	Normal = normalize(normalMatrix * aNormal);
	FragPos = vec3(model * basePosition);
	
	gl_Position = projection * view * model * basePosition;
}