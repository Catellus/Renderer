#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 camPosition;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out vec3 fragPos;
layout(location = 6) out vec3 camPos;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
	
	// Frag position in world
	fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
	
	// Normal to world space
//	mat3 trans = transpose(mat3(ubo.model));
	vec3 T  = normalize(vec3(ubo.model * vec4(inTangent, 0.0)));
	vec3 N    = normalize(vec3(ubo.model * vec4(inNormal, 0.0))); //inNormal; //normalize(inNormal * trans);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	fragTangent = T;
	fragBitangent = B;
	fragNormal = N;

//	fragBitangent = normalize(vec3(ubo.model * vec4(inBitangent, 0.0)));


	camPos = ubo.camPosition;
}

