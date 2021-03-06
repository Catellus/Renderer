
#version 450

struct DirectionalLightInfo
{
	vec3 color;
	vec3 direction;
};

struct PointLightInfo
{
	vec3 color;
	vec3 position;
	vec3 conLinQua; // The light's Constant, Linear, and Quadratic values for attenuation
};

struct SpotLightInfo
{
	vec3 color;
	vec3 position;
	vec3 direction;
	vec3 conLinQua; // The light's Constant, Linear, and Quadratic values for attenuation
	float cutOff;
	float outerCutOff;
};

// TODO : https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
layout(set = 0, binding = 1) uniform LightInfos
{
	DirectionalLightInfo directionalLight;
	PointLightInfo pointLights[4];
	SpotLightInfo spotlights[2];
}lights;
layout(set = 0, binding = 2) uniform sampler2D AlbedoSampler;
layout(set = 0, binding = 3) uniform sampler2D NormalSampler;
layout(set = 0, binding = 4) uniform sampler2D MetallicSampler;
layout(set = 0, binding = 5) uniform sampler2D RoughnessSampler;
layout(set = 0, binding = 6) uniform sampler2D AOSampler;

layout(location = 0) in vec3 inPos;			// fragment position in world-space
layout(location = 1) in vec3 inNormal;		// fragment normal in model space
layout(location = 2) in vec2 inTexCoord;	// fragment UV coordinate
layout(location = 3) in vec3 inCamPos;		// camera position in world-space

layout(location = 0) out vec4 outColor;

const float PI = 3.1415926535;

vec3 TransformNormalMapToModel()
{
	vec3 tangetNormal = texture(NormalSampler, inTexCoord).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);
	vec3 q2 = dFdy(inPos);
	vec2 st1 = dFdx(inTexCoord);
	vec2 st2 = dFdy(inTexCoord);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.x - q2 * st1.x);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangetNormal);
}

float CalculateAttenuation(vec3 conLinQua, float lightDist)
{
	return 1.0 / (conLinQua.x + conLinQua.y * lightDist + conLinQua.z * lightDist * lightDist);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num/denom;
}

float GeometrySchlickGGX(float NdotV , float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 CalculatePBRLighting(vec3 N, vec3 V, vec3 L, vec3 H, vec3 F0, vec3 albedo, float metallic, float roughness)
{
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);

	float NdotL = max(dot(N, L), 0.0);
	return (kD * albedo / PI + specular) * NdotL;
}

void main()
{
	vec3 albedo= texture(AlbedoSampler, inTexCoord).xyz;
	vec3 finalLight = vec3(0.0);
	vec3 final = vec3(0.0);

	float metallic = texture(MetallicSampler, inTexCoord).r;
	float roughness = texture(RoughnessSampler, inTexCoord).r;
	float ao = texture(AOSampler, inTexCoord).r;

	vec3 N = TransformNormalMapToModel();
	vec3 V = normalize(inCamPos - inPos);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// Directional Light
	vec3 L = normalize(-lights.directionalLight.direction);
	vec3 H = normalize(V + L);
	vec3 radiance = lights.directionalLight.color;
	finalLight += CalculatePBRLighting(N, V, L, H, F0, albedo, metallic, roughness) * radiance;

	// Point Lights
	for (int i = 0; i < 4; i++)
	{
		L = normalize(lights.pointLights[i].position - inPos);
		H = normalize(V + L);

		float dist = length(lights.pointLights[i].position - inPos);
		float attenuation = 1.0 / (dist * dist);
		vec3 radiance = lights.pointLights[i].color * attenuation;

		finalLight += CalculatePBRLighting(N, V, L, H, F0, albedo, metallic, roughness) * radiance;
	}

	// Spot Lights
	for (int i = 0; i < 2; i++)
	{
		L = normalize(lights.spotlights[i].position - inPos);
		H = normalize(V + L);

		float theta = dot(L, normalize(-lights.spotlights[i].direction));
		if (theta > lights.spotlights[i].outerCutOff)
		{
			float dist = length(lights.spotlights[i].position - inPos);
			float attenuation = 1.0 / (dist * dist);

			float epsilon = lights.spotlights[i].cutOff - lights.spotlights[i].outerCutOff;
			float coneFalloff = clamp((theta - lights.spotlights[i].outerCutOff) / epsilon, 0.0, 1.0);

			vec3 radiance = lights.spotlights[i].color * attenuation * coneFalloff;

			finalLight += CalculatePBRLighting(N, V, L, H, F0, albedo, metallic, roughness) * radiance;
		}
	}

	vec3 ambient = vec3(0.05) * albedo * ao;

	final = ambient + finalLight;
	outColor = vec4(final, 1.0);
}



