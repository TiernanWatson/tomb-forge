#version 460 core

out float FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition; // View-space position
layout(binding = 1) uniform sampler2D gNormal;   // View-space normal
layout(binding = 2) uniform sampler2D noiseTexture;

const int MAX_KERNEL_SAMPLES = 64;

uniform vec3 samples[MAX_KERNEL_SAMPLES];
uniform int kernelSize;
uniform float radius;
uniform float bias;
uniform vec2 noiseScale;
uniform mat4 projection;

void main()
{
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).xyz);
    vec3 randomVec = normalize(texture(noiseTexture, TexCoords * noiseScale).xyz);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; i++)
    {
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(gPosition, offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - sampleDepth), 0.0001));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    FragColor = occlusion;
}
