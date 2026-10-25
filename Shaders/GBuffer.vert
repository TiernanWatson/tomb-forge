#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 6) in ivec4 aBoneIds;
layout (location = 7) in vec4 aWeights;

out vec3 ViewPos;
out vec3 ViewNormal;

const int MAX_BONES = 255;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 model;

layout(std140, binding = 0) uniform BonesBlock
{
    mat4 finalBonesMatrices[MAX_BONES];
};

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

    vec4 resultPosition = vec4(0.0);
    vec3 resultNormal = vec3(0.0);
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
        resultNormal += weight * mat3(finalBonesMatrices[boneId]) * aNormal;

        wasSkinned = true;
    }

    if (!wasSkinned)
    {
        resultPosition = basePosition;
        resultNormal = aNormal;
    }

    mat4 modelView = view * model;
    mat3 normalMatrix = transpose(inverse(mat3(modelView)));

    ViewPos = vec3(modelView * resultPosition);
    ViewNormal = normalize(normalMatrix * resultNormal);

    gl_Position = projection * modelView * resultPosition;
}
