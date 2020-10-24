
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
//	DirectionalLightInfo directionalLight;
	PointLightInfo pointLights[4];
	SpotLightInfo spotlights[2];
}lights;
layout(set = 0, binding = 2) uniform sampler2D AlbedoSampler;
layout(set = 0, binding = 3) uniform sampler2D NormalSampler;

layout(location = 0) in vec3 inPos;			// fragment position in world-space
layout(location = 1) in vec3 inNormal;		// fragment normal in model space
layout(location = 2) in vec2 inTexCoord;	// fragment UV coordinate
layout(location = 3) in vec3 inCamPos;		// camera position in world-space

layout(location = 0) out vec4 outColor;

//vec3 CalculateDirectionalLightContribution(DirectionalLightInfo li, vec3 N, vec3 V)
//{
//	vec3 L = normalize(-li.direction);
//	vec3 R = reflect(-L, N);
//
//	float ambientStrength = 0.01;
//	vec3 ambient = li.color * ambientStrength;
//
//	float diffuseStrength = max(dot(N, L), 0.0);
//	vec3 diffuse = li.color * diffuseStrength;
//
//	float specularStrength = 0.5;
//	float specularity = pow(max(dot(V, R), 0.0), 32);
//	vec3 specular = li.color * specularity * specularStrength;
//
//	return (ambient + diffuse + specular) * li.color;
//}

vec3 CalculatePointLightContribution(PointLightInfo li, vec3 N, vec3 V)
{
	vec3 L = normalize(li.position - inPos);
	vec3 R = reflect(-L, N);

	float ambientStrength = 0.01;
	vec3 ambient = li.color * ambientStrength;

	float diffuseStrength = max(dot(N, L), 0.0);
	vec3 diffuse = li.color * diffuseStrength;

	float specularStrength = 0.5;
	float specularity = pow(max(dot(V, R), 0.0), 32);
	vec3 specular = li.color * specularity * specularStrength;

	float lightDist = length(li.position - inPos);
	float attenuation = 1.0 / (li.conLinQua.x + li.conLinQua.y * lightDist + li.conLinQua.z * lightDist * lightDist);

	return (ambient + diffuse + specular) * attenuation * li.color;
}

vec3 CalculateSpotlightContribution(SpotLightInfo li, vec3 N, vec3 V)
{
	vec3 L = normalize(li.position - inPos);
	vec3 R = reflect(-L, N);

	float ambientStrength = 0.01;
	vec3 ambient = li.color * ambientStrength;

	float theta = dot(L, normalize(-li.direction));
	if (theta > li.outerCutOff)
	{
		float diffuseStrength = max(dot(N, L), 0.0);
		vec3 diffuse = li.color * diffuseStrength;

		float specularStrength = 0.5;
		float specularity = pow(max(dot(V, R), 0.0), 32);
		vec3 specular = li.color * specularity * specularStrength;

		float lightDist = length(li.position - inPos);
		float attenuation = 1.0 / (li.conLinQua.x + li.conLinQua.y * lightDist + li.conLinQua.z * lightDist * lightDist);

		float epsilon   = li.cutOff - li.outerCutOff;
		float coneFalloff = clamp((theta - li.outerCutOff) / epsilon, 0.0, 1.0);

		return (ambient + ((diffuse + specular) * coneFalloff * attenuation)) * li.color;
	}
	return ambient;
}

vec3 TransformNormalMapToModel()
{
	vec3 tangetNormal = texture(NormalSampler, inTexCoord).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);
	vec3 q2 = dFdy(inPos);
	vec2 st1 = dFdx(inTexCoord);
	vec2 st2 = dFdy(inTexCoord);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.y - q2 * st1.y);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangetNormal);
}

void main()
{
	vec3 objectColor= texture(AlbedoSampler, inTexCoord).xyz;
	vec3 finalLight = vec3(0.0);
	vec3 final = vec3(0.0);

// TODO : http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
	
	vec3 N = TransformNormalMapToModel();
	vec3 V = normalize(inCamPos - inPos);

//	finalLight += CalculateDirectionalLightContribution(lights.directionalLight, N, V);

	for (int i = 0; i < 4; i++)
	{
		finalLight += CalculatePointLightContribution(lights.pointLights[i], N, V);
	}

	for (int i = 0; i < 2; i++)
	{
		finalLight += CalculateSpotlightContribution(lights.spotlights[i], N, V);
	}

	final = objectColor * finalLight;
	outColor = vec4(final, 1.0);
}



