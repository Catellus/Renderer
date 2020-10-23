
#version 450

layout(set = 0, binding = 1) uniform lightInfo {
	vec3 color;
	vec3 position;
}light;

layout(location = 0) in vec3 inPos;			// fragment position in world-space
layout(location = 1) in vec3 inNormal;		// fragment normal in model space
layout(location = 2) in vec2 inTexCoord;	// fragment UV coordinate
layout(location = 3) in vec3 inCamPos;		// camera position in world-space

layout(location = 0) out vec4 outColor;

// TODO : http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
void main()
{
	vec3 objectColor = vec3(0.7, 0.3, 1.0);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(light.position - inPos);
	vec3 V = normalize(inCamPos - inPos);
	vec3 R = reflect(-L, N);

	float ambientStrength = 0.01;
	vec3 ambient = light.color * ambientStrength;

	float diffuseStrength = max(dot(N, L), 0.0);
	vec3 diffuse = light.color * diffuseStrength;

	float specularStrength = 0.5;
	float specularity = pow(max(dot(V, R), 0.0), 32);
	vec3 specular = light.color * specularity * specularStrength;

	vec3 final = objectColor * (ambient + diffuse + specular);
	outColor = vec4(final, 1.0);
}



