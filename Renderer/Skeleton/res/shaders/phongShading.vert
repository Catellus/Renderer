#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 model;			// model-space -> world-space
	mat4 view;			// world-space -> view-space (camera-space)
	mat4 proj;			// view-space -> clip-space
	vec3 camPosition;	// Camera's position in world-space
} ubo;

layout(location = 0) in vec3 inPosition;	// vertex position in model-space
layout(location = 1) in vec2 inTexCoord;	// vertex UV coordinate
layout(location = 2) in vec3 inNormal;		// vertex normal in model-space
layout(location = 3) in vec3 inTangent;		// vertex tangent in model-space
layout(location = 4) in vec3 inBitangent;	// vertex bitangent in model-space

layout(location = 0) out vec2 outTexCoord;	// fragment UV coordinate
layout(location = 1) out vec3 outNormal;	// fragment normal in (model space?)
layout(location = 2) out vec3 outTangent;	// fragment tangent in (model space?)
layout(location = 3) out vec3 outBitangent;	// fragment bitangent in (model space?)
layout(location = 4) out vec3 outPos;		// fragment position in world-space
layout(location = 5) out vec3 outCamPos;	// camera position in world-space

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	outPos = vec3(ubo.model * vec4(inPosition, 1.0));
	outTexCoord = inTexCoord;
	outCamPos = ubo.camPosition;

	mat3 modelRotationOnly = mat3(ubo.model);
	outNormal = modelRotationOnly * inNormal;
	outTangent = modelRotationOnly * inTangent;
	outBitangent = modelRotationOnly * inBitangent;
}