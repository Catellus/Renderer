#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 model;			// model-space -> world-space
	mat4 view;			// world-space -> view-space (camera-space)
	mat4 proj;			// view-space -> clip-space
	vec3 camPosition;	// Camera's position in world-space
} ubo;

layout(location = 0) in vec3 inPosition;	// vertex position in model-space
layout(location = 1) in vec3 inNormal;		// vertex normal in model-space
layout(location = 2) in vec2 inTexCoord;	// vertex UV coordinate

layout(location = 0) out vec3 outPos;		// fragment position in world-space
layout(location = 1) out vec3 outNormal;	// fragment normal in (model space?)
layout(location = 2) out vec2 outTexCoord;	// fragment UV coordinate
layout(location = 3) out vec3 outCamPos;	// camera position in world-space

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	outPos = mat3(ubo.model) * inPosition;
	outNormal = inNormal;
	outTexCoord = inTexCoord;
	outCamPos = ubo.camPosition;
}

