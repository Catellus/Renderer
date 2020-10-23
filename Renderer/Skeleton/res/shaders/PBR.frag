#version 450

layout(set = 0, binding = 1) uniform sampler2D albetoSampler;
layout(set = 0, binding = 2) uniform lightInfo {
	vec3 color;
	vec3 position;
}light;
layout(set = 0, binding = 3) uniform sampler2D normalSampler;

layout(location = 0) in vec3 inPos;			// Frag position in world
layout(location = 1) in vec3 inNormal;		// Frag normal in world
layout(location = 2) in vec2 inTexCoord;	// Frag texel coordinate
layout(location = 3) in vec3 inCamPos;		// Position of Camera in world

layout(location = 0) out vec4 outColor;

void main()
{
// TEXTURES =================
// ==========================

	// Albeto
	vec3 albetoValue = texture(albetoSampler, inTexCoord).xyz;

	// Normal
	vec3 textureNormal = texture(normalSampler, inTexCoord).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);
	vec3 q2 = dFdy(inPos);
	vec2 st1 = dFdx(inTexCoord);
	vec2 st2 = dFdy(inTexCoord);

	vec3 N = inNormal;
	vec3 T = normalize(q1 * st2.y - q2 * st1.y);
	vec3 B = normalize(cross(N, T));
	mat3 nTBN = mat3(T, B, N);

	vec3 normal = nTBN * textureNormal;

// LIGHTS ===================
// ==========================

	vec3 lightValue = light.color; // Add falloff here

	vec3 V = normalize(inCamPos - inPos);
	vec3 L = normalize(light.position - inPos);
	vec3 H = normalize(V + L);
	vec3 R = reflect(-V, normal);

	// Ambient
	float ambientIntensity = 0.02;
	vec3 ambient = lightValue * ambientIntensity;

	// Diffuse
	vec3 diffuseLight = vec3(clamp(dot(normal, L), 0.0, 1.0)) * lightValue;

	// Specular
	float specularStrength = 0.5;
	float specularIntensity = pow(max(dot(V, R), 0.0), 32);
	vec3 specularLight = specularStrength * specularIntensity * lightValue;

	// Total light
	vec3 totalLight = ambient + diffuseLight + specularLight;

// FINAL COLOR ==============
// ==========================

	vec3 finalColor = totalLight;
	outColor = vec4(finalColor, 1.0);
}



