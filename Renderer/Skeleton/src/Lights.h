#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace skel
{
	namespace lights
	{
		struct DirectionalLight
		{
			alignas(16) glm::vec3 color;
			alignas(16) glm::vec3 direction;
		};

		struct PointLight
		{
			alignas(16) glm::vec3 color;
			alignas(16) glm::vec3 position;
			// Constant, Linear, Quadratic
			alignas(16) glm::vec3 CLQ;
		};

		struct SpotLight
		{
			alignas(16) glm::vec3 color;
			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 direction;
			// Constant, Linear, Quadratic
			alignas(16) glm::vec3 CLQ;
			alignas(4) float cutOff;
			alignas(4) float outerCutOff;
		};

// TODO : https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
		// Used as a shader uniform
		struct ShaderLights
		{
			DirectionalLight directionalLight;
			PointLight pointLights[4];
			SpotLight spotLights[2];
		};
	}
}

