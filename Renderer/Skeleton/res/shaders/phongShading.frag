
#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform lightInfo {
	vec3 color;
	vec3 direction;
}light;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 objectColor = texture(texSampler, inTexCoord).xyz;
	//vec3 finalColor =  inNormal;
	vec3 ambient = vec3(0.02, 0.02, 0.02);
	vec3 lightStuff =  vec3(max(dot(inNormal, light.direction) * light.color, 0)) + ambient;
	vec3 finalColor = objectColor * lightStuff;
	outColor = vec4(finalColor, 1.0);
}



