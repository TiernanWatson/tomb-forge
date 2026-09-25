#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 6) in ivec4 aBoneIds;
layout (location = 7) in vec4 aWeights;

const int MAX_BONES = 255;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

layout(std140, binding = 0) uniform BonesBlock
{
    mat4 finalBonesMatrices[MAX_BONES];
};

void main()
{
    vec4 basePosition = vec4(aPosition, 1.0);

    vec4 resultPosition = vec4(0.0);
    bool wasSkinned = false;

    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        int boneId = aBoneIds[i];
        float weight = aWeights[i];

        if (boneId < 0 || boneId > (MAX_BONES - 1))
        {
            continue;
        }

        resultPosition += weight * finalBonesMatrices[boneId] * basePosition;
        wasSkinned = true;
    }

    if (!wasSkinned)
    {
        resultPosition = basePosition;
    }

    gl_Position = lightSpaceMatrix * model * resultPosition;
}