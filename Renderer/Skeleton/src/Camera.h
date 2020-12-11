#pragma once

#include "Common.h"

namespace skel {

class Camera
{
// ==============================================
// Variables
// ==============================================
public:
	glm::vec3 cameraPosition;
	glm::vec3 cameraFront;
	glm::vec3 worldUp;

	float pitch = 0.0f;
	float yaw = 0.0f; // 0 = looking down the positive X axis, increases counter-clockwise

	glm::mat4 view;
	glm::mat4 projection;

// ==============================================
// Functions
// ==============================================
public:
	Camera(float _swapchainRatio)
	{
		cameraPosition = glm::vec3(0.0f, 0.0f, 6.0f);
		worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		pitch = 0.0f;
		yaw = -90.0f;
		UpdateProjection(_swapchainRatio);
		UpdateCameraView();
	}

	// Changes the projection matrix used in MVP calculations
	void UpdateProjection(float _ratio, float _VFOV = 45.0f, float _nearClipPlane = 0.1f, float _farClipPlane = 100.0f)
	{
		projection = glm::perspective(glm::radians(_VFOV), _ratio, _nearClipPlane, _farClipPlane);
		// Flip the Y axis
		projection[1][1] *= -1;
	}

	glm::vec3 getRight()
	{
		return glm::normalize(glm::cross(cameraFront, worldUp));
	}

	// Recalculates the camera's forward vector
	// Updates the View matrix
	void UpdateCameraView()
	{
		cameraFront.x = (float)(glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)));
		cameraFront.y = (float)glm::sin(glm::radians(pitch));
		cameraFront.z = (float)(glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch)));
		cameraFront = glm::normalize(cameraFront);

		view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, worldUp);
	}

}; // Camera

} // skel

