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

const int MAX_BONES = 127;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 finalBonesMatrices[MAX_BONES];

invariant gl_Position; // Keeps the position consistent if doing multiple render passes

void main()
{
	vec4 basePosition = vec4(aPosition, 1.0);

	vec4 resultPosition = vec4(0.0);
	vec3 resultNormal = vec3(0.0);
	vec3 resultTangent = vec3(0.0);
	vec3 resultBitangent = vec3(0.0);

	// todo: remove the conditional with always valid weights/data
	bool wasSkinned = false;

	// Perform skinning
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
	{
		int boneId = aBoneIds[i];
		float weight = aWeights[i];

		if (boneId < 0 || boneId > (MAX_BONES - 1))
		{
			// Less than 0 means no bone applied, but > max bones is an error
			continue;
		}

		resultPosition += weight * finalBonesMatrices[boneId] * basePosition;
		resultNormal += weight * mat3(finalBonesMatrices[boneId]) * aNormal;
		resultTangent += weight * mat3(finalBonesMatrices[boneId]) * aTangent;
		resultBitangent += weight * mat3(finalBonesMatrices[boneId]) * aBitangent;

		wasSkinned = true;
	}

	// Fallback if nothing skins this vertex (maybe this is undesired but works for this game)
	if (!wasSkinned)
	{
		resultPosition = basePosition;
		resultNormal = aNormal;
		resultTangent = aTangent;
		resultBitangent = aBitangent;
	}

	vec3 T = normalize(mat3(model) * resultTangent);
	vec3 B = normalize(mat3(model) * resultBitangent);
	vec3 N = normalize(mat3(model) * resultNormal);

	TBN = mat3(T, B, N);
	Color = aColor;
	TexCoords = aUv;
	Normal = N;
	FragPos = vec3(model * resultPosition);

	gl_Position = projection * view * model * resultPosition;
}