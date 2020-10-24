
#version 450

layout(set = 0, binding = 1) uniform lightInfo {
	vec3 color;
	vec3 position;
}light;

layout(location = 0) in vec3 inPos;		// fragment position in world-space
layout(location = 1) in vec3 inNormal;		// fragment normal in (model space?)
layout(location = 2) in vec2 inTexCoord;	// fragment UV coordinate
layout(location = 3) in vec3 inCamPos;		// camera position in world-space

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 objectColor = vec3(1.0, 1.0, 1.0);
	outColor = vec4((objectColor), 1.0);
}


