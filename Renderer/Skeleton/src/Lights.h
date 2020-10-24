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
			alignas(16) glm::vec3 ConstantLinearQuadratic;
		};

		struct SpotLight
		{
			alignas(16) glm::vec3 color;
			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 direction;
			alignas(16) glm::vec3 ConstantLinearQuadratic;
			alignas(4) float cutOff;
			alignas(4) float outerCutOff;
		};

		struct ShaderLights
		{
			//DirectionalLight directionalLight;
			PointLight pointLights[4];
			SpotLight spotLights[2];
		};
	}
}

