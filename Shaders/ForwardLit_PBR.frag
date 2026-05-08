#version 460 core
out vec4 FragColor;

in mat3 TBN;
in vec4 Color;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

#define MAX_NUMBER_OF_LIGHT_INDICES 8

struct DirLight
{
    vec3 color;
    vec3 direction;
    float strength;
};

struct PointLight
{
    vec3 position;
    vec3 color;
    float innerRadius;
    float outerRadius;
    float strength;
};

struct Material
{
    vec4 albedoColor;
    float roughnessValue;
    float metalnessValue;
};

const float PI = 3.14159265359;

uniform Material material;
uniform int lightIndices[MAX_NUMBER_OF_LIGHT_INDICES];
uniform int numLights; // Specific to the mesh being rendered
uniform int metalnessChannel; // 0 = R, 1 = G, 2 = B, 3 = A
uniform int roughnessChannel; // 0 = R, 1 = G, 2 = B, 3 = A

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

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D roughnessTexture;
layout(binding = 3) uniform sampler2D metalnessTexture;
layout(binding = 4) uniform sampler2D lightsTexture; // Each light takes up 9 texels (point lights only)

// ---------------------------------------------------------------
// Cook-Torrance PBR Functions
// ---------------------------------------------------------------

float DistributionGGX(vec3 normal, vec3 halfway, float roughness)
{
    float numerator = roughness * roughness;
    float nDotH = max(dot(normal, halfway), 0.0);
    float inner = (nDotH * nDotH) * (numerator - 1.0) + 1.0;
    float denominator = inner * inner * PI;
    return numerator / denominator;
}

float GeometrySchlickGGX(float nDotV, float k)
{
    float denominator = nDotV * (1.0 - k) + k;
    return nDotV / denominator;
}

float GeometrySmith(float nDotV, float nDotL, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return GeometrySchlickGGX(nDotV, k) * GeometrySchlickGGX(nDotL, k);
}

vec3 FresnelSchlick(float hDotV, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - hDotV, 0.0, 1.0), 5.0);
}

vec3 CookTorranceBRDF(vec3 normal, vec3 view, vec3 light, vec3 diffuse, vec3 radiance, vec3 F0, float roughness, float metalness)
{
    vec3 halfway = normalize(view + light);

    // Not clamping can cause artifacts when nDotH or nDotL is close to zero
    float nDotV = max(dot(normal, view), 0.05);
    float nDotL = max(dot(normal, light), 0.05);
    float hDotV = max(dot(halfway, view), 0.0);

    float NDF = DistributionGGX(normal, halfway, roughness);
    float G = GeometrySmith(nDotV, nDotL, roughness);
    vec3 F = FresnelSchlick(hDotV, F0);
    vec3 numerator = NDF * G * F;
    
    float denominator = 4.0 * nDotV * nDotL + 0.0001;
    vec3 specular = numerator / denominator;
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - F) * (1.0 - metalness);

    return (kD * diffuse / PI + specular) * radiance * nDotL;
}

// ---------------------------------------------------------------
// Light Processing
// ---------------------------------------------------------------

vec3 ProcessPointLight(int index, vec3 diffuse, vec3 normal, vec3 view, vec3 F0, float roughness, float metalness)
{
    // todo: do fewer texture fetches here by sampling vec4
    PointLight instance;
    instance.position.x = float(texelFetch(lightsTexture, ivec2(index * 9, 0), 0).r);
    instance.position.y = float(texelFetch(lightsTexture, ivec2(index * 9 + 1, 0), 0).r);
    instance.position.z = float(texelFetch(lightsTexture, ivec2(index * 9 + 2, 0), 0).r);
    instance.color.r = float(texelFetch(lightsTexture, ivec2(index * 9 + 3, 0), 0).r);
    instance.color.g = float(texelFetch(lightsTexture, ivec2(index * 9 + 4, 0), 0).r);
    instance.color.b = float(texelFetch(lightsTexture, ivec2(index * 9 + 5, 0), 0).r);
    instance.innerRadius = float(texelFetch(lightsTexture, ivec2(index * 9 + 6, 0), 0).r);
    instance.outerRadius = float(texelFetch(lightsTexture, ivec2(index * 9 + 7, 0), 0).r);
    instance.strength = float(texelFetch(lightsTexture, ivec2(index * 9 + 8, 0), 0).r);

    float dist = distance(FragPos, instance.position);
    float attenuation = instance.strength;
    if (dist > instance.innerRadius) // todo: remove conditional
    {
        float falloff = dist - instance.innerRadius + 1.0;
        attenuation *= 1.0 / (falloff * falloff + 0.0001);
    }
    vec3 radiance = instance.color * attenuation;
    vec3 lightDir = normalize(instance.position - FragPos);

    return CookTorranceBRDF(normal, view, lightDir, diffuse, radiance, F0, roughness, metalness);
}

vec3 ProcessDirLight(DirLight instance, vec3 albedo, vec3 normal, vec3 view, vec3 F0, float roughness, float metalness)
{
    vec3 lightDir = normalize(-instance.direction);
    float strength = max(dot(lightDir, normal), 0.0) * instance.strength;
    vec3 radiance = strength * instance.color;

    return CookTorranceBRDF(normal, view, lightDir, albedo, radiance, F0, roughness, metalness);
}

// ---------------------------------------------------------------
// Main
// ---------------------------------------------------------------

void main()
{
    vec4 albedoTex = texture(diffuseTexture, TexCoords) * material.albedoColor;
    vec3 albedo = albedoTex.rgb;
    vec3 normal = texture(normalTexture, TexCoords).rgb;
    normal = normal * 2.0 - 1.0;   
    normal = normalize(TBN * normal);
    float roughness = texture(roughnessTexture, TexCoords)[roughnessChannel] * material.roughnessValue;
    roughness = clamp(roughness, 0.05, 1.0);
    float metalness = texture(metalnessTexture, TexCoords)[metalnessChannel] * material.metalnessValue;

    vec3 view = normalize(cameraPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    DirLight dirLight;
    dirLight.color = dirLightColor;
    dirLight.direction = dirLightDirection;
    dirLight.strength = dirLightStrength;

    vec3 lights = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < numLights; i++)
    {
        lights += ProcessPointLight(lightIndices[i], albedo, normal, view, F0, roughness, metalness);
    }
    lights += ProcessDirLight(dirLight, albedo, normal, view, F0, roughness, metalness);

    vec3 ambient = ambientStrength * ambientColor;
    vec3 final = ambient + lights;
    final = final / (final + vec3(1.0)); // HDR tonemapping

    FragColor = vec4(final, albedoTex.a);
}
