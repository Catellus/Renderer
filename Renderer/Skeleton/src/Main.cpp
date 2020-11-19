#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED // Sort of breaks GLSL (ALl Zs need to be flipped)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "Renderer.h"

#include "VulkanDevice.h"
#include "ecs/ecs.h"
#include "Mesh.h"
#include "Object.h"
#include "Initializers.h"
#include "Texture.h"
#include "Lights.h"
#include "Camera.h"
#include "Shaders.h"

// TODO : Add better error handling -- Don't just throw whenever something goes wrong
class Application
{
// ==============================================
// Variables
// ==============================================
private:

// ===== Predefined =====
	const std::string SHADER_DIR  = ".\\res\\shaders\\";
	const std::string TEXTURE_DIR = ".\\res\\textures\\";
	const std::string MODEL_DIR   = ".\\res\\models\\";

	const std::string vertShaderDir = SHADER_DIR + "PBR_vert.spv";
	const std::string fragShaderDir = SHADER_DIR + "PBR_frag.spv";
	//const std::string albedoTextureDir    = TEXTURE_DIR + "CyborgWeapon\\Weapon_albedo.png";
	//const std::string normalTextureDir    = TEXTURE_DIR + "CyborgWeapon\\Weapon_normal.png";
	//const std::string metallicTextureDir  = TEXTURE_DIR + "CyborgWeapon\\Weapon_metallic.png";
	//const std::string roughnessTextureDir = TEXTURE_DIR + "CyborgWeapon\\Weapon_roughness.png";
	//const std::string aoTextureDir        = TEXTURE_DIR + "CyborgWeapon\\Weapon_ao.png";
	//const std::string testModelDir = MODEL_DIR + "CyborgWeapon\\Cyborg_Weapon.obj";

	//const std::string albedoTextureDir    = TEXTURE_DIR + "bigTextures\\spaceBlanket\\SpaceBlanket_albedo.png";
	//const std::string normalTextureDir    = TEXTURE_DIR + "bigTextures\\spaceBlanket\\SpaceBlanket_normal.png";
	//const std::string metallicTextureDir  = TEXTURE_DIR + "bigTextures\\spaceBlanket\\SpaceBlanket_metallic.png";
	//const std::string roughnessTextureDir = TEXTURE_DIR + "bigTextures\\spaceBlanket\\SpaceBlanket_roughness.png";
	//const std::string aoTextureDir        = TEXTURE_DIR + "bigTextures\\spaceBlanket\\SpaceBlanket_ao.png";
	//const std::string testModelDir = MODEL_DIR + "TestShapes\\Cube.obj";

	const std::string albedoTextureDir    = TEXTURE_DIR + "TestTextures\\White_albedo.png";
	const std::string normalTextureDir    = TEXTURE_DIR + "TestTextures\\EggCartonFoam_normal.png";
	const std::string metallicTextureDir  = TEXTURE_DIR + "TestTextures\\Blank_normal.png";
	const std::string roughnessTextureDir = TEXTURE_DIR + "TestTextures\\Blank_normal.png";
	const std::string aoTextureDir        = TEXTURE_DIR + "TestTextures\\Blank_normal.png";
	const std::string testModelDir = MODEL_DIR + "TestShapes\\SphereSmooth.obj";

	const std::string unlitVertShaderDir = SHADER_DIR + "unlit_vert.spv";
	const std::string unlitFragShaderDir = SHADER_DIR + "unlit_frag.spv";
	const std::string testTextureADir = TEXTURE_DIR + "PumpkinAlbedo.png";
	const std::string testTextureBDir = TEXTURE_DIR + "UvTest.png";
	const std::string unlitModelDir = MODEL_DIR + "TestShapes\\Sphere.obj";

	struct ApplicationTimeInformation {
		float totalTime;
		float deltaTime;
	} time;

// ===== App/Window Info =====
	const char* appTitle = "TestApp";
	const uint32_t appVersion = VK_MAKE_VERSION(0, 0, 0);
	const char* engineTitle = "TestEngine";
	const uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 0);

	uint32_t windowWidth = 1024;
	uint32_t windowHeight = 1024;
	const char* windowTitle = "Window Title";
	GLFWwindow* window;
	bool windowResized = false;

	const double mouseSensativity = 0.1f;

// ===== Shaders =====
	Renderer* renderer;

// ===== Shaders =====
	skel::shaders::ShaderDescriptorInformation opaqueShaderDescriptor;
	skel::shaders::ShaderDescriptorInformation unlitShaderDescriptor;

// ===== Objects =====
	Camera* cam;
	std::vector<Object*> testObjects;
	std::vector<Object*> bulbs;
	VkDeviceMemory* bulbColorMemory;

	skel::lights::ShaderLights finalLights;

// ==============================================
// Initialization
// ==============================================
public:

	// Called before input is handled
	void (*EarlyUpdate)(void) = NULL;
	// Called before rendering
	void (*Update)(void) = NULL;
	// Called after rendering
	void (*LateUpdate)(void) = NULL;

// Handle the lifetime of the application
	void Run()
	{
		Initialize();
		MainLoop();
		Cleanup();
	}

private:

	// Initializes all aspects required for rendering
	void Initialize()
	{
		//	testObjects.resize(4);
		bulbs.resize(4);

		// Fundamental setup
		CreateWindow();
		renderer = new Renderer(window);

		/*
		// Setup opaque shader uniform buffers
		opaqueShaderDescriptor.CreateLayoutBindingsAndPool(
			device->logicalDevice,
			{
				// MVP matrices
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
				// Lights info
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
				// Albedo map
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
				// Normal map
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
				// Metallic
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
				// Roughness
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
				// AO
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
			},
			static_cast<uint32_t>(testObjects.size())
		);

		unlitShaderDescriptor.CreateLayoutBindingsAndPool(
			device->logicalDevice,
			{
				// MVP matrices
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
				// Object color
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
				// Test texture
				//skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
			},
			static_cast<uint32_t>(bulbs.size())
		);

		//CreatePipelineLayout(unlitPipelineLayout, &unlitShaderDescriptor.descriptorSetLayout);

		for (int x = 0; x < 2; x++)
		{
			for (int y = 0; y < 2; y++)
			{
				uint32_t objectIndex = 2 * x + y;
				testObjects[objectIndex] = new Object(device, skel::shaders::ShaderTypes::Opaque, testModelDir.c_str());
				Object*& testObject = testObjects[objectIndex];

				testObject->AttachTexture(albedoTextureDir.c_str());
				testObject->AttachTexture(normalTextureDir.c_str());
				testObject->AttachTexture(metallicTextureDir.c_str());
				testObject->AttachTexture(roughnessTextureDir.c_str());
				testObject->AttachTexture(albedoTextureDir.c_str());
				testObject->AttachBuffer(sizeof(skel::lights::ShaderLights));

				opaqueShaderDescriptor.CreateDescriptorSets(device->logicalDevice, testObject->shader);
				testObject->transform.position.x = float(2 * x - 1);
				testObject->transform.position.y = float(-2 * y + 1);
				//testObject->transform.scale *= 2.f;
				testObject->transform.scale *= 0.5f;
			}
		}
		*/

		// Setup the objects in the world

		cam = new Camera(renderer->swapchainExtent.width / (float)renderer->swapchainExtent.height);

		glm::vec3 directionalColor = {0.0f, 0.0f, 0.0f};

		float toUnit = 1.0f / 255.0f;
		glm::vec3 trPointColor = {1.0f, 1.0f, 1.0f};
		glm::vec3 tlPointColor = {0.0f, 1.0f, 1.0f};
		glm::vec3 blPointColor = {1.0f, 0.0f, 1.0f};
		glm::vec3 brPointColor = {1.0f, 1.0f, 0.0f};

		glm::vec3 spotlightColor1 = {203.f/255.f, 213.f/255.f, 219.f/255.f};
		glm::vec3 spotlightColor2 = {0/255.f, 0/255.f, 0/255.f};

		// Define lighting
		// Directional light
		finalLights.directionalLight.color = directionalColor * 0.f;
		finalLights.directionalLight.direction = {0.0f, -1.0f, 0.1f};
		// Point lights
		finalLights.pointLights[0] = { trPointColor * 2.0f, {  1.0f,  1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		finalLights.pointLights[1] = { tlPointColor * 2.0f, { -1.0f,  1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		finalLights.pointLights[2] = { blPointColor * 2.0f, { -1.0f, -1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		finalLights.pointLights[3] = { brPointColor * 2.0f, {  1.0f, -1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		// Spot lights
		finalLights.spotLights[0].color = spotlightColor1 * 5.0f;
		finalLights.spotLights[0].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[0].cutOff = glm::cos(glm::radians(1.0f));
		finalLights.spotLights[0].outerCutOff = glm::cos(glm::radians(7.0f));

		finalLights.spotLights[1].color = spotlightColor2;
		finalLights.spotLights[1].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[1].cutOff = glm::cos(glm::radians(2.0f));
		finalLights.spotLights[1].outerCutOff = glm::cos(glm::radians(3.0f));

	//	for (auto& testObject : testObjects)
	//	{
	//		device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), testObject->shader.buffers[1]->memory);
	//	}

		uint32_t index = 0;
		for (auto& object : bulbs)
		{
			object = new Object(renderer->device, skel::shaders::ShaderTypes::Unlit, unlitModelDir.c_str());
			object->AttachBuffer(sizeof(glm::vec3));
			//object->AttachTexture((index %2 == 1) ? testTextureADir.c_str() : testTextureBDir.c_str());
			bulbColorMemory = &object->shader.buffers[1]->memory;
			renderer->unlitShaderDescriptor.CreateDescriptorSets(renderer->device->logicalDevice, object->shader);
			object->transform.position = finalLights.pointLights[index].position;
			object->transform.scale *= 0.05f;
			renderer->device->CopyDataToBufferMemory(&finalLights.pointLights[index].color, sizeof(glm::vec3), *bulbColorMemory);

			index++;
		}

// TODO : Create a better solution for recording arbitrary data to command buffers -- Including dynamic additions / removal
		renderer->CreateAndBeginCommandBuffers(ArbitraryCommandBufferBinding, &bulbs);

	}

	static void ArbitraryCommandBufferBinding(VkCommandBuffer& _buffer, Renderer* r, void* _context)
	{
		std::vector<Object*>& bulbs = *reinterpret_cast<std::vector<Object*>*>(_context);
		vkCmdBindPipeline(_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, r->pipeline);
		for (auto& object : bulbs)
		{
			object->Draw(_buffer, r->layout);
		}
	}

	// Initializes GLFW and creates a window
	void CreateWindow()
	{
		if (!glfwInit())
			throw std::runtime_error("Failed to init GLFW");

		// Set some window properties
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, NULL, NULL);
		if (!window)
			throw std::runtime_error("Failed to create GLFW window");

		// Setup GLFW callbacks
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
		glfwSetCursorPosCallback(window, mouseMovementCallback);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);	// Lock the cursor to the window & makes it invisible
	}

	// Called when the GLFW window is resized
	static void WindowResizeCallback(GLFWwindow* _window, int _width, int _height)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(_window));
		app->renderer->windowResized = true;
		app->cam->UpdateProjection(_width / (float)_height);
	}

	// Called when the mouse is moved within the GLFW window's borders
	// Adjusts the camera's pitch & yaw
	static void mouseMovementCallback(GLFWwindow* _window, double _x, double _y)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(_window));

		static double previousX = _x;
		static double previousY = _y;

		app->cam->yaw   += (_x - previousX) * app->mouseSensativity;
		app->cam->pitch += (previousY - _y) * app->mouseSensativity;
		if (app->cam->pitch > 89.0f)
			app->cam->pitch = 89.0f;
		if (app->cam->pitch < -89.0f)
			app->cam->pitch = -89.0f;

		previousX = _x;
		previousY = _y;
	}

// ==============================================
// Main Loop
// ==============================================

	// Core functionality
	// Loops until window is closed
	void MainLoop()
	{
		auto start = std::chrono::high_resolution_clock::now();
		auto old = start;

		while (!glfwWindowShouldClose(window))
		{
			if (EarlyUpdate)
				EarlyUpdate();

			ProcessInput();

			if (Update)
				Update();

			// Update the uniform buffers of all objects
			UpdateObjectUniforms();

			renderer->RenderFrame();

			if (LateUpdate)
				LateUpdate();

			glfwPollEvents();
			// Update the time taken for this frame, and the application's total runtime
			auto current = std::chrono::high_resolution_clock::now();
			time.deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(current - old).count();
			time.totalTime = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();
			old = current;
		}
		vkDeviceWaitIdle(renderer->device->logicalDevice);
	}

	// Interrupt called by GLFW upon input
	// Handles camera movement and sets application shutdown in motion
	void ProcessInput()
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

	// Handles rendering and presentation to the window

// TODO : define this (as a general "Update") in the Object class
	// Updates the MVP matrix for this frame
	void UpdateObjectUniforms()
	{
		finalLights.spotLights[0].position = cam->cameraPosition;
		finalLights.spotLights[0].direction = cam->cameraFront;

		finalLights.spotLights[1].position = cam->cameraPosition;
		finalLights.spotLights[1].direction = cam->cameraFront;

		//for (auto& testObject : testObjects)
		//{
		//	//testObject->transform.position.x = glm::sin(time.totalTime);
		//	//testObject->transform.rotation.y = glm::sin(time.totalTime) * 45.0f;
		//	//testObject->transform.rotation.z = glm::cos(time.totalTime * 2.0f) * 30.0f;
		//	testObject->transform.rotation.y = time.totalTime * 45.0f;
		//	testObject->transform.rotation.x = time.totalTime * 15.0f;
		//	testObject->transform.rotation.z = time.totalTime * 5.f;

		//	//testObject->transform.position.z = glm::cos(time.totalTime);
		//	testObject->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
		//	device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), testObject->shader.buffers[1]->memory);
		//}

		for (auto& object : bulbs)
		{
			object->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
		}
	}

// ==============================================
// Cleanup
// ==============================================

	// Destroys all aspects of Vulkan & GLFW
	void Cleanup()
	{
		for (auto& testObject : testObjects)
		{
			delete(testObject);
		}

		for (auto& object : bulbs)
		{
			delete(object);
		}

		//opaqueShaderDescriptor.Cleanup(device->logicalDevice);
		//unlitShaderDescriptor.Cleanup(device->logicalDevice);

		//glfwDestroyWindow(window);
		//glfwTerminate();
	}

};

int main()
{
	try
	{
		Application app;
		//app.LateUpdate = TestUpdate;
		app.Run();
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

