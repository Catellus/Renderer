#version 450

layout(set = 0, binding = 1) uniform sampler2D albetoSampler;
layout(set = 0, binding = 2) uniform lightInfo {
	vec3 color;
	vec3 position;
}light;
layout(set = 0, binding = 3) uniform sampler2D normalSampler;

layout(location = 0) in vec2 inTexCoord;	// Frag texel coordinate
layout(location = 1) in vec3 inNormal;		// Frag normal in world
layout(location = 2) in vec3 inTangent;		// Frag tangent in world
layout(location = 3) in vec3 inBitangent;	// Frag bitangent in world
layout(location = 4) in vec3 inPos;			// Frag position in world
layout(location = 5) in vec3 inCamPos;		// Position of Camera in world

layout(location = 0) out vec4 outColor;

void main()
{
// TEXTURES =================
// ==========================

	// Albeto
	vec3 albetoValue = texture(albetoSampler, inTexCoord).xyz;

	// Normal
	vec3 textureNormal = texture(normalSampler, inTexCoord).xyz;
	textureNormal = normalize(textureNormal * 2.0 - 1.0);

// LIGHTS ===================
// ==========================

	vec3 vectorToLight = normalize(vec3(light.position - inPos));
	vec3 lightValue = light.color; // Add falloff here
	vec3 viewDir = normalize(inCamPos - inPos);

	// Ambient
	float ambientIntensity = 0.02;
	vec3 ambient = lightValue * ambientIntensity;

	// Diffuse
	vec3 diffuseLight = vec3(max(dot(inNormal, vectorToLight), 0)) * lightValue;

	// Specular
	float specularStrength = 0.5;
	vec3 reflectDir = reflect(-vectorToLight, inNormal);
	float specularIntensity = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specularLight = specularStrength * specularIntensity * lightValue;

	// Total light
	vec3 totalLight = ambient + diffuseLight + specularLight;

// FINAL COLOR ==============
// ==========================

	vec3 finalColor = albetoValue * totalLight;
	outColor = vec4(finalColor, 1.0);
}



