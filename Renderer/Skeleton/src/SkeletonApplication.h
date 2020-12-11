#pragma once

#include <string>

#include "Common.h"
#include "Renderer.h"
#include "Camera.h"

namespace skel {

// TODO : Add better error handling -- Don't just throw whenever something goes wrong
class SkeletonApplication
{
// ==============================================
// STRUCTS
// ==============================================
protected:
	struct ApplicationTimeInformation {
		float totalTime = 0;
		float deltaTime = 0;
		uint32_t frameNumber = 0;
	} time;

// ==============================================
// CONSTANTS
// ==============================================

	// Window
	const int windowWidth = 800;
	const int windowHeight = 600;
	const char* windowTitle = "Test Renderer";

	// Input
	const float mouseSensativity = 0.1f;

// ==============================================
// VARIABLES
// ==============================================
protected:
	skel::Renderer* renderer;

	skel::Camera* cam;
	GLFWwindow* window;

// ==============================================
// FUNCTIONS
// ==============================================
public:
	void Run();

	//void CreateObject() {}
	//void CreateShader() {}

protected:
// VIRTUAL ======================================
// ==============================================

	// (Pure virtual) Called before the renderer is created
	// Allows renderer modification
	virtual void ChildInitialize() = 0;
	// (Pure virtual) Where everything happens
	virtual void MainLoopCore() = 0;
	// (Pure virtual) Clean up any application-specific info
	virtual void ChildCleanup() = 0;

// WINDOW =======================================
// ==============================================

	// Called when the window is resized
	static void WindowResizeCallback(GLFWwindow* _window, int _width, int _height)
	{
		auto app = reinterpret_cast<SkeletonApplication*>(glfwGetWindowUserPointer(_window));
		app->renderer->windowResized = true;
		app->cam->UpdateProjection(_width / (float)_height);
	}
	// Called when the mouse is moved within the GLFW window's borders
	// Adjusts the camera's pitch & yaw
	static void mouseMovementCallback(GLFWwindow* _window, double _x, double _y)
	{
		auto app = reinterpret_cast<SkeletonApplication*>(glfwGetWindowUserPointer(_window));

		static float previousX = (float)_x;
		static float previousY = (float)_y;
		float currentX = (float)_x;
		float currentY = (float)_y;

		app->cam->yaw += (currentX - previousX) * app->mouseSensativity;
		app->cam->pitch += (previousY - currentY) * app->mouseSensativity;
		if (app->cam->pitch > 89.0f)
			app->cam->pitch = 89.0f;
		if (app->cam->pitch < -89.0f)
			app->cam->pitch = -89.0f;

		previousX = currentX;
		previousY = currentY;
	}

// INPUT ========================================
// ==============================================

	void HandleInput();

private:
// INITIALIZATION ===============================
// ==============================================

	// Sets up the renderer
	void Initialize();
	// Initializes GLFW & creates a window
	void CreateWindow();

// MAIN LOOP ===================================
// ==============================================

	void MainLoop();

// CLEAN UP =====================================
// ==============================================

	// Clean up window & renderer
	void Cleanup();

}; // SkeletonApplication

} // namespace

