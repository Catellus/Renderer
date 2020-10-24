
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED // Sort of breaks GLSL (ALl Zs need to be flipped)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

class Camera
{
	//const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

public:
	glm::vec3 cameraPosition;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;

	double pitch = 0.0f;
	double yaw = 0.0f; // 0 = looking down the positive X axis, increases counter-clockwise

	glm::mat4 view;
	glm::mat4 projection;
private:


public:
	Camera(float _swapchainRatio)
	{
		cameraPosition = glm::vec3(0.0f, 0.0f, 6.0f);
		cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		pitch = 0.0f;
		yaw = -90.0f;
		UpdateProjection(_swapchainRatio);
		UpdateCameraView();
	}

	void UpdateProjection(float _ratio)
	{
		projection = glm::perspective(glm::radians(45.0f), _ratio, 0.1f, 100.0f);
		projection[1][1] *= -1;
	}

	glm::vec3 getRight()
	{
		return glm::normalize(glm::cross(cameraFront, cameraUp));
	}

	void UpdateCameraView()
	{
		//std::printf("%4lf, %4lf\n", pitch, yaw);

		//cameraPosition = glm::vec3(glm::sin(_time * 0.5f) * 3, 0.0f, glm::cos(_time * 0.5f) * 3);
		cameraFront.x = (float)(glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)));
		cameraFront.y = (float)glm::sin(glm::radians(pitch));
		cameraFront.z = (float)(glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch)));
		cameraFront = glm::normalize(cameraFront);

		//std::printf("%4f, %4f, %4f\n", cameraFront.r, cameraFront.g, cameraFront.b);
		view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
	}

};
