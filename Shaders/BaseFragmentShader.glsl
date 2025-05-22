#version 460 core
out vec4 FragColor;

in vec4 Color;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

#define MAX_NUMBER_OF_LIGHT_INDICES 8
#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2

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
uniform int numLights;
uniform int numDirLights;

uniform float ambientStrength;
uniform vec3 ambientColor;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D lightsTexture;

vec4 ProcessPointLight(int index);
vec4 ProcessPointLight(PointLight instance); // Deprecated
vec4 ProcessDirLight(DirLight instance);

void main()
{
	vec4 textureColor = texture(diffuseTexture, TexCoords);

	vec4 ambient = vec4(ambientStrength * ambientColor, 1.0);

	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	for (int i = 0; i < numLights; i++)
	{
		result += ProcessPointLight(lightIndices[i]);
	}

	if (numDirLights == 1)
	{
		result += ProcessDirLight(dirLight);
	}

	FragColor = (ambient + result) * textureColor;
}

vec4 ProcessPointLight(int index)
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

	float lightStrength;
	if (absDistance < instance.innerRadius)
	{
		lightStrength = 1.0;
	}
	else
	{
		lightStrength = max(mix(1.0, 0.0, (absDistance - instance.innerRadius) / (instance.outerRadius - instance.innerRadius)), 0.0) * instance.strength;
	}

	// Diffuse component
	vec3 lightDir = normalize(instance.position - FragPos);
	vec3 norm = normalize(Normal);

	float diffuseStrength = max(dot(lightDir, norm), 0.0) * lightStrength;
	vec4 diffuse = vec4(diffuseStrength * instance.color, 1.0);

	return diffuse;
}

vec4 ProcessPointLight(PointLight instance)
{
	float absDistance = distance(FragPos, instance.position);

	float lightStrength;
	if (absDistance < instance.innerRadius)
	{
		lightStrength = 1.0;
	}
	else
	{
		lightStrength = max(mix(1.0, 0.0, (absDistance - instance.innerRadius) / (instance.outerRadius - instance.innerRadius)), 0.0) * instance.strength;
	}

	// Diffuse component
	vec3 lightDir = normalize(instance.position - FragPos);
	vec3 norm = normalize(Normal);

	float diffuseStrength = max(dot(lightDir, norm), 0.0) * lightStrength;
	vec4 diffuse = vec4(diffuseStrength * instance.color, 1.0);

	return diffuse;
}

vec4 ProcessDirLight(DirLight instance)
{
	float lightStrength = instance.strength;

	// Diffuse component
	vec3 lightDir = normalize(-instance.direction);
	vec3 norm = normalize(Normal);

	float diffuseStrength = max(dot(lightDir, norm), 0.0) * lightStrength;
	vec4 diffuse = vec4(diffuseStrength * instance.color, 1.0);

	return diffuse;
}

