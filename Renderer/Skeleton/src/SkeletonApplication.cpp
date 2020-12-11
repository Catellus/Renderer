
#include <iostream>
#include <chrono>

#include "SkeletonApplication.h"

void skel::SkeletonApplication::Run()
{
	Initialize();
	ChildInitialize();
	MainLoop();
	Cleanup();
}

void skel::SkeletonApplication::HandleInput()
{
	float camSpeed = 1.0f;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camSpeed = 2.0f;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camSpeed = 0.3f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cam->cameraPosition += cam->cameraFront * camSpeed * time.deltaTime;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam->cameraPosition -= cam->cameraFront * camSpeed * time.deltaTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam->cameraPosition += cam->getRight() * camSpeed * time.deltaTime;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam->cameraPosition -= cam->getRight() * camSpeed * time.deltaTime;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		cam->cameraPosition += cam->worldUp * camSpeed * time.deltaTime;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cam->cameraPosition -= cam->worldUp * camSpeed * time.deltaTime;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	cam->UpdateCameraView();
}

void skel::SkeletonApplication::Initialize()
{
	CreateWindow();
	cam = new skel::Camera((float)windowWidth / (float)windowHeight);
	renderer = new skel::Renderer(window);
}

void skel::SkeletonApplication::CreateWindow()
{
	if (!glfwInit())
		throw std::runtime_error("Failed to init GLFW");

	// Set some window properties
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
	if (!window)
		throw std::runtime_error("Failed to create GLFW window");

	// Setup GLFW callbacks
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
	glfwSetCursorPosCallback(window, mouseMovementCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);	// Lock the cursor to the window & makes it invisible
}

void skel::SkeletonApplication::MainLoop()
{
	auto start = std::chrono::high_resolution_clock::now();
	auto old = start;
	float prevTotalTime = 0;

	while (!glfwWindowShouldClose(window))
	{
		MainLoopCore();
		glfwPollEvents();

		auto current = std::chrono::high_resolution_clock::now();
		time.deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(current - old).count();
		time.totalTime = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();
		old = current;

		if (std::floor(time.totalTime) != prevTotalTime)
		{
			prevTotalTime = std::floor(time.totalTime);
			std::printf("%5f (%4d FPS) : %6d\n", time.deltaTime, (int)(1 / time.deltaTime), time.frameNumber);
		}
		time.frameNumber++;
	}
	vkDeviceWaitIdle(renderer->device->logicalDevice);
}

void skel::SkeletonApplication::Cleanup()
{
	ChildCleanup();

	renderer->CleanupRenderer();

	glfwDestroyWindow(window);
	glfwTerminate();
}

