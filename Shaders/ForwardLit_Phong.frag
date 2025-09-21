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

uniform DirLight dirLight;
uniform int lightIndices[MAX_NUMBER_OF_LIGHT_INDICES];
uniform int numLights; // Specific to the mesh being rendered
uniform float ambientStrength;
uniform vec3 ambientColor;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D lightsTexture; // Each light takes up 9 texels (point lights only)

vec4 ProcessPointLight(int index, vec3 normal);
vec4 ProcessDirLight(DirLight instance, vec3 normal);

void main()
{
	vec3 normal = texture(normalTexture, TexCoords).rgb;
	normal.g = 1.0 - normal.g;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(TBN * normal);

	vec4 lights = vec4(0.0, 0.0, 0.0, 1.0);
	for (int i = 0; i < numLights; i++)
	{
		lights += ProcessPointLight(lightIndices[i], normal);
	}
	lights += ProcessDirLight(dirLight, normal);

	vec4 textureColor = texture(diffuseTexture, TexCoords);
	vec4 ambient = vec4(ambientStrength * ambientColor, 1.0);
	FragColor = (ambient + lights) * textureColor;
}

vec4 ProcessPointLight(int index, vec3 normal)
{
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

	float absDistance = distance(FragPos, instance.position);

	float lightStrength = instance.strength;
	if (absDistance > instance.innerRadius)
	{
		// todo: remove conditional
		float falloff = absDistance - instance.innerRadius + 1.0;
		float attenuation = 1.0 / (falloff * falloff);
		lightStrength = attenuation * instance.strength;
	}

	vec3 lightDir = normalize(instance.position - FragPos);
	float diffuseStrength = max(dot(lightDir, normal), 0.0) * lightStrength;
	return vec4(diffuseStrength * instance.color, 1.0);
}

vec4 ProcessDirLight(DirLight instance, vec3 normal)
{
	float lightStrength = instance.strength;

	vec3 lightDir = normalize(-instance.direction);
	float diffuseStrength = max(dot(lightDir, normal), 0.0) * lightStrength;
	vec4 diffuse = vec4(diffuseStrength * instance.color, 1.0);

	return max(diffuse, vec4(0.0, 0.0, 0.0, 0.0));
}

