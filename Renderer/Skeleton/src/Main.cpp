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

#include "VulkanDevice.h"
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

	const std::string vertShaderDir = SHADER_DIR + "default_vert.spv";
	const std::string fragShaderDir = SHADER_DIR + "default_frag.spv";
	const std::string unlitVertShaderDir = SHADER_DIR + "unlit_vert.spv";
	const std::string unlitFragShaderDir = SHADER_DIR + "unlit_frag.spv";
	const std::string albedoTextureDir = TEXTURE_DIR + "PumpkinAlbedo.png";
	const std::string normalTextureDir = TEXTURE_DIR + "TestNormalMap.png";
	const std::string testModelDir = MODEL_DIR + "myPumpkin.obj";
	Mesh testMesh;

	// The properties of the surface created by GLFW
	struct SurfaceProperties
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		bool isSuitable()
		{
			return formats.size() && presentModes.size();
		}
	};

	struct PipelineInformation {
		VkRenderPass renderPass;
		VkPipelineLayout layout;
		VkPipeline pipeline;
	};

	struct ApplicationTimeInformation {
		float totalTime;
		float deltaTime;
	} time;

	// Vulkan instance creation properties
	std::vector<const char*> instanceLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> instanceExtensions = {};
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const char** glfwRequiredExtentions;
	uint32_t glfwRequiredExtentionsCount;

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

// ===== Vulkan Members =====
	VkInstance instance;
	VulkanDevice* device;
	VkSurfaceKHR surface;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipelineLayout unlitPipelineLayout;
	VkPipeline pipeline;
	VkPipeline unlitPipeline;

// ===== Swapchain =====
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFrameBuffers;
	VkExtent2D swapchainExtent;
	VkFormat swapchainFormat;

// ===== Commands =====
	uint32_t graphicsCommandPoolIndex;
	std::vector<VkCommandBuffer> commandBuffers;

// ===== Presentation =====
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageIsInFlight;

// ===== Depth Image =====
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

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
		testObjects.resize(8);
		bulbs.resize(4);

		// Fundamental setup
		CreateWindow();
		CreateInstance();
		CreateSurface();
		CreateVulkanDevice();
		CreateSwapchain();
		CreateRenderpass();

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
				//skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3)
			},
			static_cast<uint32_t>(testObjects.size())
		);

		unlitShaderDescriptor.CreateLayoutBindingsAndPool(
			device->logicalDevice,
			{
				// MVP matrices
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
				// Object color
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
			},
			static_cast<uint32_t>(bulbs.size())
		);

		// Create the render pipeline
		CreatePipelineLayout(pipelineLayout, &opaqueShaderDescriptor.descriptorSetLayout);
		CreatePipelineLayout(unlitPipelineLayout, &unlitShaderDescriptor.descriptorSetLayout);
		CreateGraphicsPipeline();

		// Create the depth buffer
		CreateDepthResources();
		CreateFrameBuffers();
		CreateSyncObjects();

		// Bind render commands
		CreateCommandPools();

		// Setup the objects in the world
		cam = new Camera(swapchainExtent.width / (float)swapchainExtent.height);

		uint32_t index = 0;
		float circleIncrement = (2.0f * 3.14159f) / static_cast<uint32_t>(testObjects.size());
		for (auto& testObject : testObjects)
		{
			testObject = new Object(device, testModelDir.c_str(), skel::shaders::ShaderTypes::Opaque);
			testObject->AttachTexture(albedoTextureDir.c_str());
			//testObject->AttachTexture(normalTextureDir.c_str());
			testObject->AttachBuffer(sizeof(skel::MvpInfo));
			testObject->AttachBuffer(sizeof(skel::lights::ShaderLights));
			testObject->mvpMemory = &testObject->bufferMemories[0];
			testObject->lightBufferMemory = &testObject->bufferMemories[1];

			opaqueShaderDescriptor.CreateDescriptorSets(device->logicalDevice, testObject->shader);
			testObject->transform.position.x = glm::cos(index * circleIncrement);	// Arrange in a circle
			testObject->transform.position.y = glm::sin(index * circleIncrement);	// Arrange in a circle
			testObject->transform.scale = glm::vec3(0.3f);							// Arrange in a circle
			//testObject->transform.rotation = {0.0f, 10.0f * index, 5.0f * index};	// Arrange in a circle
			//testObject->transform.position.x = index;			// Arrange in a line
			//testObject->transform.scale = glm::vec3(0.5f);	// Arrange in a line
			index++;
		}

		// Define lighting
		// Point lights
		finalLights.pointLights[0].color = { 1.0f, 1.0f, 1.0f };
		finalLights.pointLights[0].position = { 1.0f, 1.0f, 2.0f };
		finalLights.pointLights[0].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.pointLights[1].color = { 1.0f, 0.0f, 0.0f };
		finalLights.pointLights[1].position = { -1.0f, 1.0f, 2.0f };
		finalLights.pointLights[1].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.pointLights[2].color = { 0.0f, 1.0f, 0.0f };
		finalLights.pointLights[2].position = { -1.0f, -1.0f, 2.0f };
		finalLights.pointLights[2].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.pointLights[3].color = { 0.0f, 0.0f, 1.0f };
		finalLights.pointLights[3].position = { 1.0f, -1.0f, 2.0f };
		finalLights.pointLights[3].CLQ = { 1.0f, 0.35f, 0.44f };
		// Spotlights
		finalLights.spotLights[0].color = { 1.0f, 0.2f, 0.1f };
		finalLights.spotLights[0].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[0].cutOff = glm::cos(glm::radians(2.0f));
		finalLights.spotLights[1].color = { 0.1f, 0.1f, 1.0f };
		finalLights.spotLights[1].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[1].cutOff = glm::cos(glm::radians(2.0f));

		for (auto& testObject : testObjects)
		{
			device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), *testObject->lightBufferMemory);
		}

		index = 0;
		for (auto& object : bulbs)
		{
			object = new Object(device, testModelDir.c_str(), skel::shaders::ShaderTypes::Unlit);
			object->AttachBuffer(sizeof(skel::MvpInfo));
			object->AttachBuffer(sizeof(glm::vec3));
			object->mvpMemory = &object->bufferMemories[0];
			bulbColorMemory = &object->bufferMemories[1];
			unlitShaderDescriptor.CreateDescriptorSets(device->logicalDevice, object->shader);
			object->transform.position = finalLights.pointLights[index].position;
			object->transform.scale *= 0.05f;
			//glm::vec3 bulbColor = finalLights.pointLights[index].color;
			device->CopyDataToBufferMemory(&finalLights.pointLights[index].color, sizeof(glm::vec3), *bulbColorMemory);

			index++;
		}

		// Bind render commands
		commandBuffers = CrateCommandBuffers();
	}

	// Handles changes to the swapchain for continued rendering
	void RecreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device->logicalDevice);

		// Destroy all objects that were tied to the swapchain or its images
		CleanupSwapchain();

		// Recreate all objects tied to the swapchain or its images
		CreateSwapchain();
		CreateRenderpass();
		CreatePipelineLayout(pipelineLayout, &opaqueShaderDescriptor.descriptorSetLayout);
		CreatePipelineLayout(unlitPipelineLayout, &unlitShaderDescriptor.descriptorSetLayout);
		CreateGraphicsPipeline();
		CreateDepthResources();
		CreateFrameBuffers();
		cam->UpdateProjection(swapchainExtent.width / (float)swapchainExtent.height);

		commandBuffers = CrateCommandBuffers();
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

		// Retrieve all extensions GLFW needs
		glfwRequiredExtentions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtentionsCount);
		for (uint32_t i = 0; i < glfwRequiredExtentionsCount; i++)
		{
			instanceExtensions.push_back(glfwRequiredExtentions[i]);
		}

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
		app->windowResized = true;
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

	// Make GLFW create a surface to present images to
	void CreateSurface()
	{
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
	}

	// Creates a Vulkan instance with app information
	void CreateInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = appTitle;
		appInfo.applicationVersion = appVersion;
		appInfo.pEngineName = engineTitle;
		appInfo.engineVersion = engineVersion;
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
		createInfo.ppEnabledLayerNames = instanceLayers.data();
		createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create vkInstance");
	}

	// Selects a physical device and initializes a logical device
	void CreateVulkanDevice()
	{
		// Discover GPUs in the system
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
			throw std::runtime_error("Failed to enumerate physical devices");
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// Select the best candidate
		// Get an index for each of the required queue families
		uint32_t graphicsIndex, transferIndex, presentIndex;
		uint32_t selectedDevice = ChooseSuitableDevice(devices, graphicsIndex, transferIndex, presentIndex);

		// Create a VulkanDevice & logical device
		device = new VulkanDevice(instance, devices[selectedDevice]);
		device->queueFamilyIndices.graphics = graphicsIndex;
		device->queueFamilyIndices.transfer = transferIndex;
		device->queueFamilyIndices.present = presentIndex;
		device->enabledExtensions = deviceExtensions;
		device->CreateLogicalDevice();
	}

	// Determines the suitability of all physical devices
	// Selects the first suitable device
	uint32_t ChooseSuitableDevice(std::vector<VkPhysicalDevice> _devices, uint32_t& _graphicsIndex, uint32_t& _transferIndex, uint32_t& _presentIndex)
	{
		uint32_t supportedExtensionsCount;
		std::vector<VkExtensionProperties> supportedExtensions;
		uint32_t qPropCount;
		std::vector<VkQueueFamilyProperties> qProps;
		VkPhysicalDeviceFeatures supportedFeatures;

		int graphics = -1;
		int transfer = -1;
		int present = -1;

		uint32_t i = 0;

		for (const auto& d : _devices)
		{
			// Check for the presense of all required extensions
			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
			vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, nullptr);
			supportedExtensions.resize(supportedExtensionsCount);
			vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, supportedExtensions.data());

			vkGetPhysicalDeviceFeatures(d, &supportedFeatures);

			for (const auto& e : supportedExtensions)
			{
				requiredExtensions.erase(e.extensionName);
			}

			// Find required queue familiy indices
			vkGetPhysicalDeviceQueueFamilyProperties(d, &qPropCount, nullptr);
			qProps.resize(qPropCount);
			vkGetPhysicalDeviceQueueFamilyProperties(d, &qPropCount, qProps.data());

			graphics = GetQueueFamilyIndex(qProps, VK_QUEUE_GRAPHICS_BIT);
			transfer = GetQueueFamilyIndex(qProps, VK_QUEUE_TRANSFER_BIT);
			present = GetPresentFamilyIndex(qProps, d);

			// If all required elements are present, use this device
			if (requiredExtensions.empty() && graphics >= 0 && transfer >= 0 && present >= 0 && GetSurfaceProperties(d).isSuitable() && supportedFeatures.samplerAnisotropy)
			{
				_graphicsIndex = graphics;
				_transferIndex = transfer;
				_presentIndex  = present;
				return i;
			}

			i++;
		}

		throw std::runtime_error("Failed to find a suitable physical device");
		return 0;
	}

	// Returns the index of the first queue with desired properties
	uint32_t GetQueueFamilyIndex(std::vector<VkQueueFamilyProperties> _families, VkQueueFlags _flag)
	{
		uint32_t bestFit = UINT32_MAX;
		for (uint32_t i = 0; i < (uint32_t)_families.size(); i++)
		{
			if (_flag & _families[i].queueFlags)
			{
				if (_flag == VK_QUEUE_TRANSFER_BIT) // Append additional queue families here (ex: compute)
				{
					if ((_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
						return i;
					else
						bestFit = i;
				}
				else
					return i;
			}
		}

		if (bestFit == UINT32_MAX)
			throw std::runtime_error("Failed to find a queue index");
		return bestFit;
	}

	// Returns the index of the first queue that supports the surface
	// Queries for WSI support
	int GetPresentFamilyIndex(std::vector<VkQueueFamilyProperties> qProps, VkPhysicalDevice d)
	{
		uint32_t tmpQueueProp = 0;
		for (const auto& queueProp : qProps)
		{
			VkBool32 supported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(d, tmpQueueProp, surface, &supported);
			if (supported == VK_TRUE)
			{
				return tmpQueueProp;
			}
			tmpQueueProp++;
		}

		return -1;
	}

	// Returns the device's supported image and presentation information
	SurfaceProperties GetSurfaceProperties(VkPhysicalDevice _device)
	{
		SurfaceProperties properties;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, surface, &properties.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface, &formatCount, nullptr);
		properties.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface, &formatCount, properties.formats.data());

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface, &presentModeCount, nullptr);
		properties.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface, &presentModeCount, properties.presentModes.data());

		return properties;
	}

	// Create the basic command pool for rendering
	void CreateCommandPools()
	{
		graphicsCommandPoolIndex = device->CreateCommandPool(device->queueFamilyIndices.graphics, 0);
		device->graphicsCommandPoolIndex = graphicsCommandPoolIndex;
	}

	// Creates the Vulkan Swapchain object
	void CreateSwapchain()
	{
		SurfaceProperties properties = GetSurfaceProperties(device->physicalDevice);
		VkSurfaceFormatKHR format;
		VkPresentModeKHR presentMode;
		VkExtent2D extent;
		GetIdealSurfaceProperties(properties, format, presentMode, extent);

		uint32_t imageCount = properties.capabilities.minImageCount + 1;
		if (properties.capabilities.maxImageCount > 0 && imageCount > properties.capabilities.maxImageCount)
		{
			imageCount = properties.capabilities.maxImageCount;
		}

		// Define the properties of the swapchain and its images
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.presentMode = presentMode;
		createInfo.imageExtent = extent;
		createInfo.preTransform = properties.capabilities.currentTransform;
		createInfo.minImageCount = imageCount;
		createInfo.surface = surface;
		createInfo.clipped = VK_TRUE;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.imageArrayLayers = 1;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

// TODO : Use only exclusive sharing & handle handoffs manually (using pipeline barriers)
		if (device->queueFamilyIndices.graphics == device->queueFamilyIndices.present)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		else
		{
			uint32_t sharingIndices[] = {device->queueFamilyIndices.graphics, device->queueFamilyIndices.present};

			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = sharingIndices;
		}

		if (vkCreateSwapchainKHR(device->logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swapchain");
		}

		// Retrieve the swapchain's images
		// Create an imageView for each
		vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, nullptr);
		swapchainImages.resize(imageCount);
		swapchainImageViews.resize(imageCount);
		vkGetSwapchainImagesKHR(device->logicalDevice, swapchain, &imageCount, swapchainImages.data());

		swapchainExtent = extent;
		swapchainFormat = format.format;

		for (uint32_t i = 0; i < imageCount; i++)
		{
			swapchainImageViews[i] = skel::CreateImageView(device, swapchainImages[i], swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	// Finds a surface with ideal format, present mode, and extent properties
	void GetIdealSurfaceProperties(SurfaceProperties _properties, VkSurfaceFormatKHR& _format, VkPresentModeKHR& _presentMode, VkExtent2D& _extent)
	{
		// Search for SRGB and 32 bit color capabilities
		_format = _properties.formats[0];
		for (const auto& f : _properties.formats)
		{
			if (f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && f.format == VK_FORMAT_B8G8R8A8_SRGB)
			{
				_format = f;
				break;
			}
		}

		// FIFO is guaranteed
		_presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& m : _properties.presentModes)
		{
			if (m == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				_presentMode = m;
				break;
			}
		}

		// Define the extent if one is not present
		if (_properties.capabilities.currentExtent.width != UINT32_MAX)
		{
			_extent = _properties.capabilities.currentExtent;
		}
		else
		{
			int tmpWidth, tmpHeight;
			glfwGetFramebufferSize(window, &tmpWidth, &tmpHeight);
			VkExtent2D actual = { (uint32_t)tmpWidth, (uint32_t)tmpHeight };
			actual.width = glm::clamp(actual.width, _properties.capabilities.minImageExtent.width, _properties.capabilities.maxImageExtent.width);
			actual.height = glm::clamp(actual.height, _properties.capabilities.minImageExtent.height, _properties.capabilities.maxImageExtent.height);
			_extent = actual;
		}
	}

	// Specifies the image view(s) to bind to renderpass attachments
	void CreateFrameBuffers()
	{
		swapchainFrameBuffers.resize((uint32_t)swapchainImages.size());

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.width = swapchainExtent.width;
		createInfo.height = swapchainExtent.height;
		createInfo.renderPass = renderPass;
		createInfo.layers = 1;

		for (uint32_t i = 0; i < (uint32_t)swapchainImages.size(); i++)
		{
			std::array<VkImageView, 2> attachments = { swapchainImageViews[i], depthImageView};
			createInfo.attachmentCount = (uint32_t)attachments.size();
			createInfo.pAttachments = attachments.data();

			if (vkCreateFramebuffer(device->logicalDevice, &createInfo, nullptr, &swapchainFrameBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer");
		}
	}

	// Specify what types of attachments will be accessed by the graphics pipeline
	void CreateRenderpass()
	{
		// Describes the swapchain images' color
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapchainFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Describes the depth texture
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachementReference = {};
		depthAttachementReference.attachment = 1;
		depthAttachementReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Single required subpass for these attachments
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentReference;
		subpass.pDepthStencilAttachment = &depthAttachementReference;

		// No memory dependencies between subpasses
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = (uint32_t)attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device->logicalDevice, &createInfo, nullptr, &renderPass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create renderpass");
	}

	// Define the properties for each stage of the graphics pipeline
	void CreateGraphicsPipeline()
	{
	// ===== Viewport =====
		VkViewport viewport;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.extent = swapchainExtent;
		scissor.offset = { 0, 0 };

		VkPipelineViewportStateCreateInfo viewportState =
			skel::initializers::PipelineViewportStateCreateInfo(
				1,
				&viewport,
				1,
				&scissor
			);

	// ===== Vertex Input =====
	// Vertex information (Position, Color, UV, etc)
		VkVertexInputBindingDescription vertInputBinding = Vertex::GetBindingDescription();
		auto vertInputAttribute = Vertex::GetAttributeDescription();

		VkPipelineVertexInputStateCreateInfo vertInputState =
			skel::initializers::PipelineVertexInputStateCreateInfo(
				1,
				&vertInputBinding,
				static_cast<uint32_t>(vertInputAttribute.size()),
				vertInputAttribute.data()
			);

	// ===== Input Assembly =====
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			skel::initializers::PipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
			);

	// ===== Rasterizer =====
		VkPipelineRasterizationStateCreateInfo rasterizationState =
			skel::initializers::PipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL
			);

	// ===== Multisampling =====
		VkPipelineMultisampleStateCreateInfo mutisampleState =
			skel::initializers::PipelineMultisampleStateCreateInfo(
				VK_SAMPLE_COUNT_1_BIT
			);

	// ===== Depth Stencil =====
		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			skel::initializers::PipelineDepthStencilStateCreateInfo(
				VK_TRUE,
				VK_TRUE,
				VK_COMPARE_OP_LESS
			);

	// ===== Color Blend =====
		VkPipelineColorBlendAttachmentState colorBlendAttachment =
			skel::initializers::PipelineColorBlendAttachmentState(
				VK_FALSE,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			);

		VkPipelineColorBlendStateCreateInfo colorBlendState =
			skel::initializers::PipelineColorBlendStateCreateInfo(
				1,
				&colorBlendAttachment
			);

	// ===== Dynamic =====
	// States of the pipeline that change frequently (ex: the viewport resizing)
		VkPipelineDynamicStateCreateInfo dynamicState =
			skel::initializers::PipelineDynamicStateCreateInfo(
				0,
				nullptr
			);

	// ===== Pipeline Layout =====
	// Defines the shader's expected uniform bindings
	// Moved to Initialization

	// ===== Shader Stage =====
	// Opaque =====
		std::vector<char> vertFile = LoadFile(vertShaderDir.c_str());
		std::vector<char> fragFile = LoadFile(fragShaderDir.c_str());
		VkShaderModule vertShaderModule = CreateShaderModule(vertFile);
		VkShaderModule fragShaderModule = CreateShaderModule(fragFile);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo =
			skel::initializers::PipelineShaderStageCreateInfo(
				VK_SHADER_STAGE_VERTEX_BIT,
				vertShaderModule
			);
		VkPipelineShaderStageCreateInfo fragShaderStageInfo =
			skel::initializers::PipelineShaderStageCreateInfo(
				VK_SHADER_STAGE_FRAGMENT_BIT,
				fragShaderModule
			);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
			vertShaderStageInfo,
			fragShaderStageInfo
		};

	// Unlit =====
		std::vector<char> unlitVertFile = LoadFile(unlitVertShaderDir.c_str());
		std::vector<char> unlitFragFile = LoadFile(unlitFragShaderDir.c_str());
		VkShaderModule unlitVertShaderModule = CreateShaderModule(unlitVertFile);
		VkShaderModule unlitFragShaderModule = CreateShaderModule(unlitFragFile);

		VkPipelineShaderStageCreateInfo unlitVertShaderStageInfo =
			skel::initializers::PipelineShaderStageCreateInfo(
				VK_SHADER_STAGE_VERTEX_BIT,
				unlitVertShaderModule
			);
		VkPipelineShaderStageCreateInfo unlitFragShaderStageInfo =
			skel::initializers::PipelineShaderStageCreateInfo(
				VK_SHADER_STAGE_FRAGMENT_BIT,
				unlitFragShaderModule
			);

		std::vector<VkPipelineShaderStageCreateInfo> unlitShaderStages = {
			unlitVertShaderStageInfo,
			unlitFragShaderStageInfo
		};

	// ===== Pipeline Creation =====
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = 
			skel::initializers::GraphicsPipelineCreateInfo(
				pipelineLayout,
				renderPass
			);

		pipelineCreateInfo.pViewportState		= &viewportState;
		pipelineCreateInfo.pVertexInputState	= &vertInputState;
		pipelineCreateInfo.pInputAssemblyState	= &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState	= &rasterizationState;
		pipelineCreateInfo.pMultisampleState	= &mutisampleState;
		pipelineCreateInfo.pDepthStencilState	= &depthStencilState;
		pipelineCreateInfo.pColorBlendState		= &colorBlendState;
		pipelineCreateInfo.pDynamicState		= &dynamicState;


		// Create the opaque-shadered pipeline =====
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		if (vkCreateGraphicsPipelines(device->logicalDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create opaque pipeline");

		vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);

		// Create the unlit-shadered pipeline =====
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(unlitShaderStages.size());
		pipelineCreateInfo.pStages = unlitShaderStages.data();
		pipelineCreateInfo.layout = unlitPipelineLayout;

		if (vkCreateGraphicsPipelines(device->logicalDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &unlitPipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create unlit pipeline");

		vkDestroyShaderModule(device->logicalDevice, unlitVertShaderModule, nullptr);
		vkDestroyShaderModule(device->logicalDevice, unlitFragShaderModule, nullptr);
	}

	// Binds shader uniforms
	void CreatePipelineLayout(VkPipelineLayout& _pipelineLayout, VkDescriptorSetLayout* _shaderLayouts, uint32_t _count = 1)
	{
		VkPipelineLayoutCreateInfo createInfo =
			skel::initializers::PipelineLayoutCreateInfo(
				_shaderLayouts,
				_count
			);

		if (vkCreatePipelineLayout(device->logicalDevice, &createInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline layout");
	}

	// Create a pipeline usable object for shader code
	VkShaderModule CreateShaderModule(const std::vector<char> _code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = (uint32_t)_code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(_code.data());

		VkShaderModule tmpShaderModule;
		if (vkCreateShaderModule(device->logicalDevice, &createInfo, nullptr, &tmpShaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module");

		return tmpShaderModule;
	}

	// Loads a file as binary
	// Returns a character vector
	std::vector<char> LoadFile(const char* _directory)
	{
		std::ifstream stream;
		stream.open(_directory, std::ios::ate | std::ios::binary);
		if (!stream.is_open())
			throw std::runtime_error("Failed to open shader directory");

		uint32_t size = (uint32_t)stream.tellg();
		stream.seekg(0);

		std::vector<char> bytes(size);
		stream.read(bytes.data(), size);

		stream.close();
		return bytes;
	}

	// Creates a set of semaphores and a fence for every frame
	void CreateSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imageIsInFlight.resize((uint32_t)swapchainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device->logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]))
				throw std::runtime_error("Failed to create semaphore");
		}
	}

// ==============================================
// Depth/Stencil image
// ==============================================

	// Creates an image and view for the depth texture
	void CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat();

		skel::CreateImage(
			device,
			swapchainExtent.width,
			swapchainExtent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage,
			depthImageMemory
			);
		depthImageView = skel::CreateImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	// Returns the depth texture's image format
	VkFormat FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	// Returns the first format of the desired properties supported by the GPU
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& _candidates, VkImageTiling _tiling, VkFormatFeatureFlags _features)
	{
		for (VkFormat format : _candidates)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &properties);

			if (_tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & _features) == _features)
				return format;
			else if (_tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & _features) == _features)
				return format;
		}
		throw std::runtime_error("Failed to find a suitable format");
	}

// ==============================================
// Command Buffers
// ==============================================

	// Allocates space for, creates, and returns a set of rendering command buffers
	std::vector<VkCommandBuffer> CrateCommandBuffers()
	{
		std::vector<VkCommandBuffer> cmdBuffers;
		uint32_t cmdBufferCount = AllocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, graphicsCommandPoolIndex, cmdBuffers);

		std::array<VkClearValue, 2> clearValues;
		clearValues[0].color = { 0.02f, 0.025f, 0.03f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = swapchainExtent;
		renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();

		for (uint32_t i = 0; i < cmdBufferCount; i++)
		{
			// Begin recording a command
			if (vkBeginCommandBuffer(cmdBuffers[i], &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("Failed to begin command buffer");

			renderPassBeginInfo.framebuffer = swapchainFrameBuffers[i];
			vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Opaque pipeline
			vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			for (auto& testObject : testObjects)
			{
				testObject->Draw(cmdBuffers[i], pipelineLayout);
			}

			// Unlit pipeline
			vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipeline);
			for (auto& object : bulbs)
			{
				object->Draw(cmdBuffers[i], unlitPipelineLayout);
			}

			// Complete the recording
			EndCommandBuffer(cmdBuffers[i]);
		}

		return cmdBuffers;
	}

	// Allocate memory for command recording
	uint32_t AllocateCommandBuffers(VkCommandBufferLevel _level, uint32_t _commandPoolIndex, std::vector<VkCommandBuffer>& _buffers)
	{
		uint32_t size = (uint32_t)swapchainFrameBuffers.size();
		_buffers.resize(size);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = device->commandPools[_commandPoolIndex];
		allocInfo.level = _level;
		allocInfo.commandBufferCount = size;

		if (vkAllocateCommandBuffers(device->logicalDevice, &allocInfo, _buffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command buffers");

		return size;
	}

	void EndCommandBuffer(VkCommandBuffer& _buffer)
	{
		vkCmdEndRenderPass(_buffer);

		if (vkEndCommandBuffer(_buffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer");
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
			ProcessInput();

			// Update the uniform buffers of all objects
			UpdateObjectUniforms();

			RenderFrame();
			glfwPollEvents();

			// Update the time taken for this frame, and the application's total runtime
			auto current = std::chrono::high_resolution_clock::now();
			time.deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(current - old).count();
			time.totalTime = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();
			old = current;
		}
		vkDeviceWaitIdle(device->logicalDevice);
	}

	// Interrupt called by GLFW upon input
	// Handles camera movement and sets application shutdown in motion
	void ProcessInput()
	{
		float camSpeed = 1.0f;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			camSpeed = 2.0f;
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
	void RenderFrame()
	{
		uint32_t imageIndex;

		// Wait for and retrieve the next frame for rendering
		vkWaitForFences(device->logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		VkResult result = vkAcquireNextImageKHR(device->logicalDevice, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swapchain image");
		}

		if (imageIsInFlight[imageIndex] != VK_NULL_HANDLE)
			vkWaitForFences(device->logicalDevice, 1, &imageIsInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		imageIsInFlight[imageIndex] = inFlightFences[currentFrame];

		// Define render command submittal synchronization elements
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = { renderCompleteSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device->logicalDevice, 1, &inFlightFences[currentFrame]);

		// Render this frame
		if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer");

		// Define presentation command submittal synchronization elements
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &imageIndex;

		// Present this frame
		result = vkQueuePresentKHR(device->presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized)
		{
			windowResized = false;
			RecreateSwapchain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swapchain");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

// TODO : define this (as a general "Update") in the Object class
	// Updates the MVP matrix for this frame
	void UpdateObjectUniforms()
	{
		finalLights.spotLights[0].position = cam->cameraPosition;
		finalLights.spotLights[0].direction = cam->cameraFront;
		finalLights.spotLights[0].outerCutOff = glm::cos(glm::radians(5.0f + glm::cos(time.totalTime)));
		finalLights.spotLights[1].position = cam->cameraPosition;
		finalLights.spotLights[1].direction = cam->cameraFront;
		finalLights.spotLights[1].outerCutOff = glm::cos(glm::radians(5.0f + glm::cos(time.totalTime + 3.14159f)));

		for (auto& testObject : testObjects)
		{
			//testObject->transform.position.x = glm::sin(time.totalTime);
			//testObject->transform.rotation.y = glm::sin(time.totalTime) * 30.0f;
			//testObject->transform.rotation.z = glm::cos(time.totalTime) * 30.0f;

			testObject->transform.position.z = glm::cos(time.totalTime);
			testObject->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
			device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), *testObject->lightBufferMemory);
		}

		for (auto& object : bulbs)
		{
			object->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
		}
	}

// ==============================================
// Cleanup
// ==============================================

	// Destroys all aspects of the swapchain
	void CleanupSwapchain()
	{
		vkDestroyImageView(device->logicalDevice, depthImageView, nullptr);
		vkDestroyImage(device->logicalDevice, depthImage, nullptr);
		vkFreeMemory(device->logicalDevice, depthImageMemory, nullptr);

		for (const auto& f : swapchainFrameBuffers)
			vkDestroyFramebuffer(device->logicalDevice, f, nullptr);

		vkFreeCommandBuffers(device->logicalDevice, device->commandPools[graphicsCommandPoolIndex], (uint32_t)commandBuffers.size(), commandBuffers.data());

		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
		vkDestroyPipeline(device->logicalDevice, unlitPipeline, nullptr);
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
		vkDestroyPipelineLayout(device->logicalDevice, unlitPipelineLayout, nullptr);
		vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);

		for (const auto& v : swapchainImageViews)
			vkDestroyImageView(device->logicalDevice, v, nullptr);

		vkDestroySwapchainKHR(device->logicalDevice, swapchain, nullptr);
	}

	// Destroys all aspects of Vulkan & GLFW
	void Cleanup()
	{
		CleanupSwapchain();

		for (auto& testObject : testObjects)
		{
			delete(testObject);
		}

		for (auto& object : bulbs)
		{
			delete(object);
		}

		opaqueShaderDescriptor.Cleanup(device->logicalDevice);
		unlitShaderDescriptor.Cleanup(device->logicalDevice);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device->logicalDevice, renderCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device->logicalDevice, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device->logicalDevice, inFlightFences[i], nullptr);
		}

		device->Cleanup();

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

};

int main()
{
	try
	{
		Application app;
		app.Run();
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

