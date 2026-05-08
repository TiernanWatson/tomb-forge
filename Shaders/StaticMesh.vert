#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aBitangent;
layout (location = 4) in vec4 aColor;
layout (location = 5) in vec2 aUv;
layout (location = 6) in ivec4 aBoneIds; 
layout (location = 7) in vec4 aWeights;

out mat3 TBN;
out vec4 Color;
out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;

// Shared UBO data for every frame. Must match the renderer struct.
layout(std140, binding = 1) uniform PerFrameData
{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    float pad0;
    vec3 ambientColor;
    float ambientStrength;
    vec3 dirLightColor;
    float dirLightStrength;
    vec3 dirLightDirection;
    float pad1;
};

void main()
{
	vec4 basePosition = vec4(aPosition, 1.0);
	mat3 normalMatrix = transpose(inverse(mat3(model)));

	vec3 N = normalize(normalMatrix * aNormal);
	vec3 T = normalize(normalMatrix * aTangent);
	vec3 B = normalize(normalMatrix * aBitangent);

	TBN = mat3(T, B, N);
	Color = aColor;
	Normal = N;
	FragPos = vec3(model * basePosition);
	TexCoords = aUv;
	
	gl_Position = projection * view * model * basePosition;
}