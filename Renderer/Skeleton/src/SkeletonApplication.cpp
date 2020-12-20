
#include <iostream>
#include <chrono>

#include "SkeletonApplication.h"

void skel::SkeletonApplication::Run()
{
	Initialize();
	MainLoop();
	Cleanup();
}

void skel::SkeletonApplication::WindowResizeListener(SDL_Window* _window)
{
	int width, height;
	SDL_GetWindowSize(_window, &width, &height);

	std::printf("New window size: (%d, %d)\n", width, height);
	cam->UpdateProjection((float)width/(float)height);
}

void skel::SkeletonApplication::HandleInput()
{
	SDL_Event e;
	float camSpeed = 1.0f;
	while (SDL_PollEvent(&e) != 0)
	{
		// Closes the application
		if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
			applicationShouldClose = true;

		// Adjusts the camera's pitch & yaw
		if (e.type == SDL_MOUSEMOTION)
		{
			cam->yaw += e.motion.xrel * mouseSensativity;
			cam->pitch -= e.motion.yrel * mouseSensativity;
			if (cam->pitch > 89.f)
				cam->pitch = 89.f;
			if (cam->pitch < -89.f)
				cam->pitch = -89.f;
		}

		if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == 1)
		{
			static uint32_t x = e.button.x, y = e.button.y;

			SDL_bool shouldHold = static_cast<SDL_bool>(!SDL_GetRelativeMouseMode());
			SDL_SetWindowGrab(window, shouldHold);
			SDL_SetRelativeMouseMode(shouldHold);

			// Puts the mouse back where it started
			if (shouldHold == SDL_TRUE)
			{
				x = e.button.x;
				y = e.button.y;
			}
			else
			{
				// Triggers a relatively large mouseMotion event
				SDL_WarpMouseInWindow(window, x, y);
			}
		}
	}

	int count = 0;
	const Uint8* keys = SDL_GetKeyboardState(&count);
	if (keys[SDL_SCANCODE_LSHIFT] == SDL_PRESSED)
		camSpeed = 2.0f;
	if (keys[SDL_SCANCODE_LCTRL] == SDL_PRESSED)
		camSpeed = 0.3f;

	if (keys[SDL_SCANCODE_W] == SDL_PRESSED)
		cam->cameraPosition += cam->cameraFront * camSpeed * time.deltaTime;
	if (keys[SDL_SCANCODE_S] == SDL_PRESSED)
		cam->cameraPosition -= cam->cameraFront * camSpeed * time.deltaTime;

	if (keys[SDL_SCANCODE_D] == SDL_PRESSED)
		cam->cameraPosition += cam->getRight() * camSpeed * time.deltaTime;
	if (keys[SDL_SCANCODE_A] == SDL_PRESSED)
		cam->cameraPosition -= cam->getRight() * camSpeed * time.deltaTime;

	if (keys[SDL_SCANCODE_E] == SDL_PRESSED)
		cam->cameraPosition += cam->worldUp * camSpeed * time.deltaTime;
	if (keys[SDL_SCANCODE_Q] == SDL_PRESSED)
		cam->cameraPosition -= cam->worldUp * camSpeed * time.deltaTime;

	cam->UpdateCameraView();
}

void skel::SkeletonApplication::Initialize()
{
	CreateWindow();
	cam = new skel::Camera((float)windowWidth / (float)windowHeight);
	renderer = new skel::Renderer(window, cam);
	ChildInitialize();
	renderer->Initialize();
}

void skel::SkeletonApplication::CreateWindow()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::printf("SDL error: %s", SDL_GetError());
		throw std::runtime_error("Failed to init SDL");
	}

	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;

	window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, flags);
	if (window == nullptr)
	{
		std::printf("SDL error: %s", SDL_GetError());
		throw std::runtime_error("Failed to create SDL window");
	}

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_SetWindowGrab(window, SDL_TRUE);
}

void skel::SkeletonApplication::MainLoop()
{
	auto start = std::chrono::high_resolution_clock::now();
	auto old = start;
	float prevTotalTime = 0;

	while (!applicationShouldClose)
	{
		MainLoopCore();

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
	free(renderer);

	free(cam);

	SDL_DestroyWindow(window);
	SDL_Quit();
}

