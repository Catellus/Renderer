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
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb/stb_image.h>

#include "VulkanDevice.h"
#include "Mesh.h"
#include "Object.h"
#include "Initializers.h"
#include "Texture.h"
#include "Lights.h"
#include "Camera.h"

// TODO : Fix normal re-calculation (Normals spiraling @ top/bottom of sphere)

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
	const std::string albedoTextureDir = TEXTURE_DIR + "UvTest.png";
	const std::string normalTextureDir = TEXTURE_DIR + "TestNormalMap.png";
	const std::string testModelDir = MODEL_DIR + "Cube.obj";
	Mesh testMesh;

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

	struct ApplicationTimeInformation {
		float totalTime;
		float deltaTime;
	} time;

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

	uint32_t windowWidth = 800;
	uint32_t windowHeight = 600;
	const char* windowTitle = "Window Title";
	GLFWwindow* window;
	bool windowResized = false;

// ===== Vulkan Members =====
	VkInstance instance;
	VulkanDevice* device;
	VkSurfaceKHR surface;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkPipeline unlitPipeline;

	uint32_t graphicsCommandPoolIndex;
	std::vector<VkCommandBuffer> commandBuffers;

// ===== Swapchain & Presentation =====
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFrameBuffers;
	VkExtent2D swapchainExtent;
	VkFormat swapchainFormat;

	const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageIsInFlight;

// ===== Shaders =====
	// Depth
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

// ===== Objects =====
	Camera* cam;

	std::vector<Object*> testObjects;
	Object* Bulb;

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

		CreateWindow();
		CreateInstance();
		CreateSurface();
		CreateVulkanDevice();
		CreateSwapchain();
		CreateRenderpass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateDescriptorPool(static_cast<uint32_t>(testObjects.size()) + 1);
		CreateCommandPools();
		// Depth buffer
		CreateDepthResources();
		CreateFrameBuffers();
		CreateSyncObjects();

		cam = new Camera(swapchainExtent.width / (float)swapchainExtent.height);

		uint32_t index = 0;
		float circleIncrement = (2.0f * 3.14159f) / static_cast<uint32_t>(testObjects.size());
		for (auto& testObject : testObjects)
		{
			testObject = new Object(device, testModelDir.c_str(), albedoTextureDir.c_str(), normalTextureDir.c_str());
			testObject->CreateDescriptorSets(descriptorPool, descriptorSetLayout);
			testObject->transform.position.x = glm::cos(index * circleIncrement);
			testObject->transform.position.y = glm::sin(index * circleIncrement);
			testObject->transform.scale = glm::vec3(0.3f);
			testObject->transform.rotation = {0.0f, 10.0f * index, 5.0f * index};
			//testObject->transform.position.x = index;
			//testObject->transform.scale = glm::vec3(0.5f);
			index++;
		}

		Bulb = new Object(device, testModelDir.c_str(), albedoTextureDir.c_str(), normalTextureDir.c_str());
		Bulb->CreateDescriptorSets(descriptorPool, descriptorSetLayout);
		Bulb->transform.scale *= 0.1f;

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

		CleanupSwapchain();

		CreateSwapchain();
		CreateRenderpass();
		CreateGraphicsPipeline();
		CreateDepthResources();
		CreateFrameBuffers();
		CreateDescriptorPool(static_cast<uint32_t>(testObjects.size()) + 1);
		for (auto& testObject : testObjects)
		{
			testObject->CreateDescriptorSets(descriptorPool, descriptorSetLayout);
		}
		Bulb->CreateDescriptorSets(descriptorPool, descriptorSetLayout);
		cam->UpdateProjection(swapchainExtent.width / (float)swapchainExtent.height);

		commandBuffers = CrateCommandBuffers();
	}

	#pragma region Object

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;

// ==============================================
// Uniform Buffers
// ==============================================

	// Describe the layout of the descriptor set				(CreateDescriptorSetLayout)
	// Attach the descriptor set layout to the pipeline layout	(CreatePipelineLayout)
	//		Create the buffer to store data						(CreateUniformBuffers)
	// Specify the size of the uniform							(CreateDescriptorPool)
	// Write basic information into the descriptor sets			(CreateDescriptorSets)
	// Bind descriptor set in a command buffer					(CreateCommandBUffers)

	void CreateDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
			skel::initializers::DescriptorSetLyoutBinding(	// MVP matrices
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0
			),
			skel::initializers::DescriptorSetLyoutBinding(	// Light infos
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1
			),
			skel::initializers::DescriptorSetLyoutBinding(	// Albedo map
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2
			),
			skel::initializers::DescriptorSetLyoutBinding(	// Normal map
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				3
			)
		};

		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = (uint32_t)layoutBindings.size();
		createInfo.pBindings = layoutBindings.data();

		if (vkCreateDescriptorSetLayout(device->logicalDevice, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a descriptor set layout");
	}

	// Pool sizes are determined by the shaders in the pipeline
	void CreateDescriptorPool(uint32_t _objectCount)
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _objectCount),			// UBO
			skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _objectCount),			// Light
			skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _objectCount),	// Albeto Map
			skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _objectCount)		// Normal Map
		};

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = (uint32_t)poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		createInfo.maxSets = _objectCount;

		if (vkCreateDescriptorPool(device->logicalDevice, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool");
	}
	#pragma endregion

	// Creates a GLFW window pointer
	void CreateWindow()
	{
		if (!glfwInit())
			throw std::runtime_error("Failed to init GLFW");
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, NULL, NULL);
		if (!window)
			throw std::runtime_error("Failed to create GLFW window");

		glfwRequiredExtentions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtentionsCount);
		for (uint32_t i = 0; i < glfwRequiredExtentionsCount; i++)
		{
			instanceExtensions.push_back(glfwRequiredExtentions[i]);
		}

		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
		glfwSetCursorPosCallback(window, mouseMovementCallback);
		// Lock the cursor to the window & makes it invisible
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	// Called when the GLFW window is resized
	static void WindowResizeCallback(GLFWwindow* _window, int _width, int _height)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(_window));
		app->windowResized = true;
	}

	const double mouseSensativity = 0.1f;
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

	// Selects a physical device and initializes logical device
	void CreateVulkanDevice()
	{
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
			throw std::runtime_error("Failed to enumerate physical devices");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		uint32_t graphicsIndex, transferIndex, presentIndex;
		uint32_t selectedDevice = ChooseSuitableDevice(devices, graphicsIndex, transferIndex, presentIndex);

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
			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
			vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, nullptr);
			supportedExtensions.resize(supportedExtensionsCount);
			vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, supportedExtensions.data());

			vkGetPhysicalDeviceFeatures(d, &supportedFeatures);

			for (const auto& e : supportedExtensions)
			{
				requiredExtensions.erase(e.extensionName);
			}

			vkGetPhysicalDeviceQueueFamilyProperties(d, &qPropCount, nullptr);
			qProps.resize(qPropCount);
			vkGetPhysicalDeviceQueueFamilyProperties(d, &qPropCount, qProps.data());

			graphics = GetQueueFamilyIndex(qProps, VK_QUEUE_GRAPHICS_BIT);
			transfer = GetQueueFamilyIndex(qProps, VK_QUEUE_TRANSFER_BIT);
			present = GetPresentFamilyIndex(qProps, d);

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
				if (_flag == VK_QUEUE_TRANSFER_BIT)
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
	int GetPresentFamilyIndex(std::vector<VkQueueFamilyProperties> qProps, VkPhysicalDevice d)
	{
		uint32_t tmpQueueProp = 0;
		for (const auto& queueProp : qProps)
		{
			VkBool32 support = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(d, tmpQueueProp, surface, &support);
			if (support == VK_TRUE)
			{
				return tmpQueueProp;
			}
			tmpQueueProp++;
		}

		return -1;
	}

	// Returns the device's surface capabilities
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

	// Create basic command pools for rendering
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

	// Finds the best fit for the surface's format, present mode, and extent
	void GetIdealSurfaceProperties(SurfaceProperties _properties, VkSurfaceFormatKHR& _format, VkPresentModeKHR& _presentMode, VkExtent2D& _extent)
	{
		_format = _properties.formats[0];
		for (const auto& f : _properties.formats)
		{
			if (f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && f.format == VK_FORMAT_B8G8R8A8_SRGB)
			{
				_format = f;
				break;
			}
		}

		_presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& m : _properties.presentModes)
		{
			if (m == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				_presentMode = m;
				break;
			}
		}

		if (_properties.capabilities.currentExtent.width != UINT32_MAX)
		{
			_extent = _properties.capabilities.currentExtent;
		}
		else
		{
			int tmpWidth, tmpHeight;
			glfwGetFramebufferSize(window, &tmpWidth, &tmpHeight);
			VkExtent2D actual = { (uint32_t)tmpWidth, (uint32_t)tmpHeight };
			actual.width = clamp(actual.width, _properties.capabilities.minImageExtent.width, _properties.capabilities.maxImageExtent.width);
			actual.height = clamp(actual.height, _properties.capabilities.minImageExtent.height, _properties.capabilities.maxImageExtent.height);
			_extent = actual;
		}
	}

// TODO : Move this to a math library
	// Keep an input number between a min & max
	uint32_t clamp(uint32_t _val, uint32_t _min, uint32_t _max)
	{
		if (_val < _min)
			return _min;
		if (_val > _max)
			return _max;
		return _val;
	}

	// Create framebuffers
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

	// Specify what types of attachments will be accessed
	void CreateRenderpass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapchainFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachementReference = {};
		depthAttachementReference.attachment = 1;
		depthAttachementReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentReference;
		subpass.pDepthStencilAttachment = &depthAttachementReference;

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

	// Define variables for each stage of the pipeline
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
		VkPipelineDynamicStateCreateInfo dynamicState =
			skel::initializers::PipelineDynamicStateCreateInfo(
				0,
				nullptr
			);

	// ===== Pipeline Layout =====
		CreatePipelineLayout();

	// ===== Shader Stage =====
		// Normal =====
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


		// Normal =====
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		if (vkCreateGraphicsPipelines(device->logicalDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);

		// Unlit =====
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(unlitShaderStages.size());
		pipelineCreateInfo.pStages = unlitShaderStages.data();

		if (vkCreateGraphicsPipelines(device->logicalDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &unlitPipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create unlit pipeline");

		vkDestroyShaderModule(device->logicalDevice, unlitVertShaderModule, nullptr);
		vkDestroyShaderModule(device->logicalDevice, unlitFragShaderModule, nullptr);
	}

	// Binds shader uniforms
	void CreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo createInfo =
			skel::initializers::PipelineLayoutCreateInfo(
				&descriptorSetLayout
			);

		if (vkCreatePipelineLayout(device->logicalDevice, &createInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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

	// Loads a file as a binary file
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
// Create Depth test
// ==============================================

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

	VkFormat FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

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

	bool HasStencilComponent(VkFormat _format)
	{
		return _format == VK_FORMAT_D32_SFLOAT_S8_UINT || _format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

// ==============================================
// Load models
// ==============================================

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
			//BeginCommandBuffer(i, renderPass, cmdBuffers[i]);

			if (vkBeginCommandBuffer(cmdBuffers[i], &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("Failed to begin command buffer");

			renderPassBeginInfo.framebuffer = swapchainFrameBuffers[i];

			vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			for (auto& testObject : testObjects)
			{
				testObject->Draw(cmdBuffers[i], pipelineLayout);
			}

			vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipeline);
			Bulb->Draw(cmdBuffers[i], pipelineLayout);

			EndCommandBuffer(cmdBuffers[i]);
		}

		return cmdBuffers;
	}

// ==============================================
// Command Buffers
// ==============================================

	// Create objects with instructions on rendering
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

	void BeginCommandBuffer(uint32_t _index, VkRenderPass _renderPass, VkCommandBuffer& _buffer)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(_buffer, &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to begin command buffer");

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = _renderPass;
		renderPassBeginInfo.framebuffer = swapchainFrameBuffers[_index];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = swapchainExtent;
		std::array<VkClearValue, 2> clearValues;
		clearValues[0].color = { 0.02f, 0.025f, 0.03f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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
			RenderFrame();
			glfwPollEvents();

			auto current = std::chrono::high_resolution_clock::now();
			time.deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(current - old).count();
			time.totalTime = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();
			old = current;
		}
		vkDeviceWaitIdle(device->logicalDevice);
	}

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
			cam->cameraPosition += cam->cameraUp * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			cam->cameraPosition -= cam->cameraUp * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		cam->UpdateCameraView();
	}

	// Handles rendering and presentation to the window
	void RenderFrame()
	{
		vkWaitForFences(device->logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
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

		UpdateObjectUniforms(imageIndex);

		if (imageIsInFlight[imageIndex] != VK_NULL_HANDLE)
			vkWaitForFences(device->logicalDevice, 1, &imageIsInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		imageIsInFlight[imageIndex] = inFlightFences[currentFrame];

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
		if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &imageIndex;

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

	// Updates the MVP matrix for this frame
	void UpdateObjectUniforms(uint32_t _currentFrame)
	{
		//testObject.transform.position.x = glm::sin(time.totalTime);
		//testObject.transform.rotation.y = glm::sin(time.totalTime) * 30.0f;
		//testObject.transform.rotation.z = glm::cos(time.totalTime) * 30.0f;
		for (auto& testObject : testObjects)
		{
			testObject->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
		}

		//light.color = { glm::sin(time.totalTime * 2.0f), glm::sin(time.totalTime * 0.7f), glm::sin(time.totalTime * 1.3f)};
		//light.position = {0.0f, glm::sin(time.totalTime), (glm::cos(time.totalTime) * 0.5f + 1.5f)};
		//light.position = {glm::cos(time.totalTime) * 3.0f, -1.0f, glm::sin(time.totalTime) * 3.0f};
		//light.position = {0.0f, 0.0f, 5.5f};
		//light.direction = {0.0f, 0.0f, -1.0f};

	// LIGHT
		skel::lights::ShaderLights finalLights;
		// Points
		finalLights.pointLights[0].color = {1.0f, 1.0f, 1.0f};
		finalLights.pointLights[0].position = {1.0f, 1.0f, 1.0f};
		finalLights.pointLights[0].ConstantLinearQuadratic = {1.0f, 0.35f, 0.44f};
		finalLights.pointLights[1].color = { 1.0f, 0.0f, 0.0f };
		finalLights.pointLights[1].position = { -1.0f, 1.0f, 1.0f };
		finalLights.pointLights[1].ConstantLinearQuadratic = { 1.0f, 0.35f, 0.44f };
		finalLights.pointLights[2].color = { 0.0f, 1.0f, 0.0f };
		finalLights.pointLights[2].position = { -1.0f, -1.0f, 1.0f };
		finalLights.pointLights[2].ConstantLinearQuadratic = { 1.0f, 0.35f, 0.44f };
		finalLights.pointLights[3].color = { 0.0f, 0.0f, 1.0f };
		finalLights.pointLights[3].position = { 1.0f, -1.0f, 1.0f };
		finalLights.pointLights[3].ConstantLinearQuadratic = { 1.0f, 0.35f, 0.44f };
		// Spots
		finalLights.spotLights[0].color = { 1.0f, 0.2f, 0.1f };
		finalLights.spotLights[0].position = cam->cameraPosition;
		finalLights.spotLights[0].direction = cam->cameraFront;
		finalLights.spotLights[0].ConstantLinearQuadratic = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[0].cutOff = glm::cos(glm::radians(2.0f));
		finalLights.spotLights[0].outerCutOff = glm::cos(glm::radians(5.0f + glm::cos(time.totalTime)));
		finalLights.spotLights[1].color = { 0.1f, 0.1f, 1.0f };
		finalLights.spotLights[1].position = cam->cameraPosition;
		finalLights.spotLights[1].direction = cam->cameraFront;
		finalLights.spotLights[1].ConstantLinearQuadratic = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[1].cutOff = glm::cos(glm::radians(2.0f));
		finalLights.spotLights[1].outerCutOff = glm::cos(glm::radians(5.0f + glm::cos(time.totalTime + 3.14159f)));

		for (auto& testObject : testObjects)
		{
			device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), testObject->lightBufferMemory);
		}
		device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), Bulb->lightBufferMemory);

		//Bulb->transform.position = finalLights.spotLights[0].position;
		Bulb->transform.position = {0.0f, 0.0f, 0.0f};
		Bulb->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
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

		vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);

		for (const auto& f : swapchainFrameBuffers)
			vkDestroyFramebuffer(device->logicalDevice, f, nullptr);

		vkFreeCommandBuffers(device->logicalDevice, device->commandPools[graphicsCommandPoolIndex], (uint32_t)commandBuffers.size(), commandBuffers.data());

		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
		vkDestroyPipeline(device->logicalDevice, unlitPipeline, nullptr);
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
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

		delete(Bulb);

		vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);

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

