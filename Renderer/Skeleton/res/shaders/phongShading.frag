#version 450

layout(set = 0, binding = 1) uniform sampler2D albetoSampler;
layout(set = 0, binding = 2) uniform lightInfo {
	vec3 color;
	vec3 direction;
}light;
layout(set = 0, binding = 3) uniform sampler2D normalSampler;

layout(location = 0) in vec3 inColor;		// 
layout(location = 1) in vec2 inTexCoord;	// Frag texel coordinate
layout(location = 2) in vec3 inNormal;		// Frag normal in world
layout(location = 3) in vec3 fragPos;		// Frag position in world
layout(location = 4) in vec3 camPos;		// Position of Camera in world

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 objectColor = texture(albetoSampler, inTexCoord).xyz;
	
	vec3 vectorToLight = normalize(vec3(light.direction - fragPos));
	/*
	// Ambient
	vec3 ambient = vec3(0.02, 0.02, 0.02);
	// Diffuse
	vec3 normalizedNormal = normalize(inNormal);
	vec3 diffuseLight =  vec3(max(dot(normalizedNormal, vectorToLight), 0)) * light.color;
	// Specular
	float specularStrength = 0.5;
	vec3 viewDir = normalize(camPos - fragPos);
	vec3 reflectDir = reflect(-vectorToLight, inNormal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specularLight = specularStrength * spec * light.color;
	// Final color
	vec3 totalLight = ambient + diffuseLight + specularLight;
	//vec3 totalLight = ambient + specularLight;
	vec3 finalColor = objectColor * totalLight;
	//vec3 finalColor =  inNormal;
	*/
	
	outColor = vec4(finalColor, 1.0);
}



