#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec2 aUv;
layout (location = 4) in ivec4 aBoneIds; 
layout (location = 5) in vec4 aWeights;

out vec4 Color;
out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 finalBonesMatrices[MAX_BONES];

invariant gl_Position;

void main()
{
	vec4 basePosition = vec4(aPosition, 1.0f);

	vec4 resultPosition = vec4(0.0f);
	vec3 resultNormal = vec3(0.0f);

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

		vec4 partialPosition = finalBonesMatrices[boneId] * basePosition;
		resultPosition += weight * partialPosition;

		vec3 partialNormal = mat3(finalBonesMatrices[boneId]) * aNormal;
		resultNormal += weight * partialNormal;

		wasSkinned = true;
	}

	// Fallback if nothing skins this vertex (maybe this is undesired but works for this game)
	if (!wasSkinned)
	{
		resultPosition = basePosition;
		resultNormal = aNormal;
	}

	Color = aColor;
	TexCoords = aUv;
	Normal = mat3(model) * resultNormal;
	FragPos = vec3(model * resultPosition);

	gl_Position = projection * view * model * resultPosition;
}