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
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "VulkanDevice.h"
#include "FileLoader.h"
#include "Mesh.h"

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

	const std::string vertShaderDir = SHADER_DIR + "phongShading_vert.spv";
	const std::string fragShaderDir = SHADER_DIR + "phongShading_frag.spv";
	const std::string albedoTextureDir = TEXTURE_DIR + "White.png";
	const std::string normalTextureDir = TEXTURE_DIR + "gold\\Metal_GoldOld_1K_normal.png";
	const std::string testModelDir = MODEL_DIR + "SphereSmooth.obj";
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

	// Mind the alignment
	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::vec3 camPosition;
	};

	struct LightInformation {
		alignas(16) glm::vec3 color;
		alignas(16) glm::vec3 position;
	};

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

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	uint32_t graphicsCommandPoolIndex;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<Vertex> vertices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	std::vector<uint32_t> indices;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uboBuffers;
	std::vector<VkDeviceMemory> uboBuffersMemory;
	std::vector<VkBuffer> lightBuffers;
	std::vector<VkDeviceMemory> lightBuffersMemory;

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
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	// Albedo
	VkImage albedoImage;
	VkDeviceMemory albedoImageMemory;
	VkImageView albedoImageView;
	VkSampler albedoImageSampler;
	// Normal
	VkImage normalImage;
	VkDeviceMemory normalImageMemory;
	VkImageView normalImageView;
	VkSampler normalImageSampler;
	// Depth
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

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
		CreateWindow();
		CreateInstance();
		CreateSurface();
		CreateVulkanDevice();
		CreateSwapchain();
		CreateRenderpass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateCommandPools();
	// Textures
		CreateTexture(albedoTextureDir, albedoImage, albedoImageMemory, albedoImageView, albedoImageSampler);
		CreateTexture(normalTextureDir, normalImage, normalImageMemory, normalImageView, normalImageSampler);
	// Depth buffer
		CreateDepthResources();
	// 
		CreateFrameBuffers();
	// Model
		testMesh = LoadModel(testModelDir);
		std::printf("%s\n\tVerts: %d\n\tIndices: %d\n", testModelDir.c_str(), (uint32_t)testMesh.vertices.size(), (uint32_t)testMesh.indices.size());
	// Shader inputs
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
	// 
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		CreateSyncObjects();
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
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
	}

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
		// "Capture" the cursor when the window has focus
		// Locks the cursor to the window & makes it invisible
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

		app->cam.yaw   += (_x - previousX) * app->mouseSensativity;
		app->cam.pitch += (previousY - _y) * app->mouseSensativity;
		if (app->cam.pitch > 89.0f)
			app->cam.pitch = 89.0f;
		if (app->cam.pitch < -89.0f)
			app->cam.pitch = -89.0f;

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
			swapchainImageViews[i] = CreateImageView(swapchainImages[i], swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT);
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
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// ===== Shader Stage =====
		std::vector<char> vertFile = LoadFile(vertShaderDir.c_str());
		std::vector<char> fragFile = LoadFile(fragShaderDir.c_str());
		vertShaderModule = CreateShaderModule(vertFile);
		fragShaderModule = CreateShaderModule(fragFile);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;

	// ===== Vertex Input =====
		VkVertexInputBindingDescription vertInputBinding = Vertex::GetBindingDescription();
		auto vertInputAttribute = Vertex::GetAttributeDescription();

		VkPipelineVertexInputStateCreateInfo vertInputState = {};
		vertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertInputState.vertexBindingDescriptionCount = 1;
		vertInputState.pVertexBindingDescriptions = &vertInputBinding;
		vertInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertInputAttribute.size());
		vertInputState.pVertexAttributeDescriptions = vertInputAttribute.data();

		pipelineCreateInfo.pVertexInputState = &vertInputState;

	// ===== Input Assembly =====
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;

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

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		pipelineCreateInfo.pViewportState = &viewportState;

	// ===== Rasterizer =====
		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.depthClampEnable = VK_FALSE;
		//rasterizationState.depthBiasClamp = 0;
		//rasterizationState.depthBiasConstantFactor = 0;
		//rasterizationState.depthBiasSlopeFactor = 0;
		rasterizationState.lineWidth = 1;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;

		pipelineCreateInfo.pRasterizationState = &rasterizationState;

	// ===== Multisampling =====
		VkPipelineMultisampleStateCreateInfo mutisampleState = {};
		mutisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		mutisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		mutisampleState.sampleShadingEnable = VK_FALSE;
		//mutisampleState.alphaToCoverageEnable = VK_FALSE;
		//mutisampleState.alphaToOneEnable = VK_FALSE;

		pipelineCreateInfo.pMultisampleState = &mutisampleState;

	// ===== Depth Stencil =====
		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilState.stencilTestEnable = VK_FALSE;

		pipelineCreateInfo.pDepthStencilState = &depthStencilState;

	// ===== Color Blend =====
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		pipelineCreateInfo.pColorBlendState = &colorBlendState;

	// ===== Dynamic =====
		pipelineCreateInfo.pDynamicState = nullptr;

	// ===== Pipeline Layout =====
		CreatePipelineLayout();
		pipelineCreateInfo.layout = pipelineLayout;

	// ===== RenderPass =====
		pipelineCreateInfo.renderPass = renderPass;

	// ===== Pipeline Creation =====

		if (vkCreateGraphicsPipelines(device->logicalDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);
	}

	// Binds shader uniforms
	void CreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = 1;
		createInfo.pSetLayouts = &descriptorSetLayout;

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

	// Creates a buffer in GPU memory for the Verts vector
	// Copies the Verts vector into the buffer
	void CreateVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(testMesh.vertices[0]) * (uint32_t)testMesh.vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		device->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, testMesh.vertices.data(), (uint32_t)bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingBufferMemory);
		//CopyDataToBufferMemory(testMesh.vertices.data(), bufferSize);

		device->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			vertexBuffer,
			vertexBufferMemory
		);

		device->CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingBufferMemory, nullptr);
	}

	// Creates a buffer in GPU memory for the Indices vector
	// Copies the Indices vector into the buffer
	void CreateIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(testMesh.indices[0]) * (uint32_t)testMesh.indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		device->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, testMesh.indices.data(), (uint32_t)bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingBufferMemory);

		device->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			indexBuffer,
			indexBufferMemory
		);

		device->CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingBufferMemory, nullptr);
	}

// ==============================================
// Uniform Buffers
// ==============================================

	// Describe the layout of the descriptor set				(CreateDescriptorSetLayout)
	// Attach the descriptor set layout to the pipeline layout	(CreatePipelineLayout)
	//		Create the buffer to store data						(CreateUniformBuffers)
	// Specify the size of the uniform							(CreateDescriptorPool)
	// Write basic information into the descriptor sets			(CreateDescriptorSets)
	// Bind descriptor set in a command buffer					(CreateCommandBUffers)

	// Define --
// TODO : Generalize this
	void CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding albedoMapLayoutBinding = {};
		albedoMapLayoutBinding.binding = 1;
		albedoMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoMapLayoutBinding.descriptorCount = 1;
		albedoMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		albedoMapLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding lightLayoutBinding = {};
		lightLayoutBinding.binding = 2;
		lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightLayoutBinding.descriptorCount = 1;
		lightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		lightLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding normalMapLayoutBinding = {};
		normalMapLayoutBinding.binding = 3;
		normalMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalMapLayoutBinding.descriptorCount = 1;
		normalMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		normalMapLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, albedoMapLayoutBinding, lightLayoutBinding, normalMapLayoutBinding };
		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = (uint32_t)bindings.size();
		createInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device->logicalDevice, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create a descriptor set layout");
	}

	void CreateUniformBuffers()
	{
		VkDeviceSize uboBufferSize = sizeof(UniformBufferObject);
		uboBuffers.resize((uint32_t)swapchainImages.size());
		uboBuffersMemory.resize((uint32_t)swapchainImages.size());

		VkDeviceSize lightBufferSize = sizeof(LightInformation);
		lightBuffers.resize((uint32_t)swapchainImages.size());
		lightBuffersMemory.resize((uint32_t)swapchainImages.size());

		for (uint32_t i = 0; i < (uint32_t)swapchainImages.size(); i++)
		{
			device->CreateBuffer(
				uboBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uboBuffers[i],
				uboBuffersMemory[i]
			);

			device->CreateBuffer(
				lightBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				lightBuffers[i],
				lightBuffersMemory[i]
			);
		}
	}

	void CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 4> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;				// UBO
		poolSizes[0].descriptorCount = (uint32_t)swapchainImages.size();	// UBO
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;		// albedo map sampler
		poolSizes[1].descriptorCount = (uint32_t)swapchainImages.size();	// albedo map sampler
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;				// Light Information
		poolSizes[2].descriptorCount = (uint32_t)swapchainImages.size();	// Light Information
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;		// normal map sampler
		poolSizes[3].descriptorCount = (uint32_t)swapchainImages.size();	// normal map sampler

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = (uint32_t)poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		createInfo.maxSets = (uint32_t)swapchainImages.size();

		if (vkCreateDescriptorPool(device->logicalDevice, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool");
	}

	void CreateDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts((uint32_t)swapchainImages.size(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)swapchainImages.size();
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize((uint32_t)swapchainImages.size());
		if (vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor sets");

		for (uint32_t i = 0; i < (uint32_t)swapchainImages.size(); i++)
		{
			VkDescriptorBufferInfo uboInfo = {};
			uboInfo.offset = 0;
			uboInfo.range = sizeof(UniformBufferObject);
			uboInfo.buffer = uboBuffers[i];

			VkDescriptorImageInfo albedoInfo = {};
			albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			albedoInfo.imageView = albedoImageView;
			albedoInfo.sampler = albedoImageSampler;

			VkDescriptorBufferInfo lightInfo = {};
			lightInfo.offset = 0;
			lightInfo.range = sizeof(LightInformation);
			lightInfo.buffer = lightBuffers[i];

			VkDescriptorImageInfo normalInfo = {};
			normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			normalInfo.imageView = normalImageView;
			normalInfo.sampler = normalImageSampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};
			// UBO
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &uboInfo;
			// Albedo sampler
			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &albedoInfo;
			// UBO
			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &lightInfo;
			// Normal sampler
			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &normalInfo;

			vkUpdateDescriptorSets(device->logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

// ==============================================
// Command Buffers
// ==============================================

	// Create objects with instructions on rendering
// TODO : Split like Begin/End Single Time Commands
	void CreateCommandBuffers()
	{
		commandBuffers.resize((uint32_t)swapchainFrameBuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = device->commandPools[graphicsCommandPoolIndex];
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device->logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command buffers");

		for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("Failed to begin command buffer");

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = swapchainFrameBuffers[i];
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = swapchainExtent;
			std::array<VkClearValue, 2> clearValues;
			clearValues[0].color = { 0.02f, 0.025f, 0.03f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			VkBuffer vertBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(testMesh.indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to record command buffer");
		}
	}

// ==============================================
// Create texture
// ==============================================

	void CreateTexture(const std::string _dir, VkImage& _image, VkDeviceMemory& _imageMemory, VkImageView& _imageView, VkSampler& _imageSampler)
	{
		CreateTextureImage(_dir, _image, _imageMemory);
		CreateTextureImageView(_image, _imageView);
		CreateTextureSampler(_imageSampler);
	}

	void CreateTextureImage(const std::string _directory, VkImage& _image, VkDeviceMemory& _imageMemory)
	{
		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = stbi_load(_directory.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = textureWidth * textureHeight * 4;

		if (!pixels)
			throw std::runtime_error("Failed to load texture image");

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		device->CreateBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		CopyDataToBufferMemory(pixels, (size_t)imageSize, stagingBufferMemory);
		stbi_image_free(pixels);

		CreateImage(
			(uint32_t)textureWidth,
			(uint32_t)textureHeight,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			_image,
			_imageMemory
		);

		TransitionImageLayout(_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, _image, (uint32_t)textureWidth, (uint32_t)textureHeight);
		TransitionImageLayout(_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingBufferMemory, nullptr);
	}

	uint32_t FindMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((_typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
				return i;
		}

		throw std::runtime_error("Failed to find suitable memory type");
	}

	void CreateImage(uint32_t _width, uint32_t _height, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageMemory)
	{
		VkImageCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.extent.width = _width;
		createInfo.extent.height = _height;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.format = _format;
		createInfo.tiling = _tiling;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.usage = _usage;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device->logicalDevice, &createInfo, nullptr, &_image) != VK_SUCCESS)
			throw std::runtime_error("Failed to crate image");

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device->logicalDevice, _image, &memoryRequirements);
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, _properties);

		if (vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate image memory");

		vkBindImageMemory(device->logicalDevice, _image, _imageMemory, 0);
	}

	VkImageView CreateImageView(VkImage _image, VkFormat _format, VkImageAspectFlags _aspectFlags)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = _image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = _format;
		viewInfo.subresourceRange.aspectMask = _aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view");

		return imageView;
	}

	void CopyBufferToImage(VkBuffer _buffer, VkImage _image, uint32_t _width, uint32_t _height)
	{
		VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands(device->transientPoolIndex);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = {_width, _height, 1};

		vkCmdCopyBufferToImage(commandBuffer, _buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		device->EndSingleTimeCommands(commandBuffer, device->transientPoolIndex, device->transferQueue);
	}

	void CreateTextureImageView(VkImage& _image, VkImageView& _imageView)
	{
		_imageView = CreateImageView(_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void CreateTextureSampler(VkSampler& _imageSampler)
	{
		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = 16.0f;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.0f;

		if (vkCreateSampler(device->logicalDevice, &createInfo, nullptr, &_imageSampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler");
	}

	void TransitionImageLayout(VkImage _image, VkFormat _format, VkImageLayout _oldLayout, VkImageLayout _newLayout)
	{
		VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands(graphicsCommandPoolIndex);

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = _oldLayout;
		barrier.newLayout = _newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // For Queue family transfer
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // For Queue family transfer
		barrier.image = _image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
			throw std::invalid_argument("Unsupported layout transition");

		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		device->EndSingleTimeCommands(commandBuffer, graphicsCommandPoolIndex, device->graphicsQueue);
	}

// ==============================================
// Create Depth test
// ==============================================

	void CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat();

		CreateImage(
			swapchainExtent.width,
			swapchainExtent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage,
			depthImageMemory
			);
		depthImageView = CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
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

	

// ==============================================
// Main Loop
// ==============================================

	#include "Camera.h"
	Camera cam;

	struct ApplicationTimeInformation {
		float totalTime;
		float deltaTime;
	} time;

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
		const float camSpeed = 0.5f;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			cam.cameraPosition += cam.cameraFront * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			cam.cameraPosition -= cam.cameraFront * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			cam.cameraPosition += cam.getRight() * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			cam.cameraPosition -= cam.getRight() * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			cam.cameraPosition += cam.cameraUp * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			cam.cameraPosition -= cam.cameraUp * camSpeed * time.deltaTime;
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		cam.UpdateCameraView();
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

		UpdateUniformBuffer(imageIndex);

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
	void UpdateUniformBuffer(uint32_t _currentFrame)
	{
	// UBO
		UniformBufferObject ubo;

		ubo.view = cam.view;
		ubo.camPosition = cam.cameraPosition;

		ubo.model = glm::rotate(glm::mat4(1.0f), time.totalTime * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		//ubo.model = glm::mat4(1.0f);
		ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 100.0f);
		ubo.proj[1][1] *= -1;

		CopyDataToBufferMemory(&ubo, sizeof(ubo), uboBuffersMemory[_currentFrame]);

	// LIGHT
		LightInformation light = {};
		light.color = {1.0f, 1.0f, 1.0f};
		//light.position = {0.0f, glm::sin(time.totalTime), (glm::cos(time.totalTime) * 0.5f + 1.5f)};
		light.position = {0.0f, 0.0f, 3.0f};

		CopyDataToBufferMemory(&light, sizeof(LightInformation), lightBuffersMemory[_currentFrame]);
	}

	// Maps buffer memory and copies input data to it
	void CopyDataToBufferMemory(const void* _srcData, VkDeviceSize _size, VkDeviceMemory& _memory, VkDeviceSize _offset = 0, VkMemoryMapFlags _flags = 0)
	{
		void* tmpData;
		vkMapMemory(device->logicalDevice, _memory, _offset, _size, _flags, &tmpData);
		memcpy(tmpData, _srcData, _size);
		vkUnmapMemory(device->logicalDevice, _memory);
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

		for (uint32_t i = 0; i < (uint32_t)swapchainImages.size(); i++)
		{
			vkDestroyBuffer(device->logicalDevice, uboBuffers[i], nullptr);
			vkFreeMemory(device->logicalDevice, uboBuffersMemory[i], nullptr);
			vkDestroyBuffer(device->logicalDevice, lightBuffers[i], nullptr);
			vkFreeMemory(device->logicalDevice, lightBuffersMemory[i], nullptr);
		}
		vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);

		for (const auto& f : swapchainFrameBuffers)
			vkDestroyFramebuffer(device->logicalDevice, f, nullptr);

		vkFreeCommandBuffers(device->logicalDevice, device->commandPools[graphicsCommandPoolIndex], (uint32_t)commandBuffers.size(), commandBuffers.data());

		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
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

		vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);

		// Albedo
		vkDestroySampler(device->logicalDevice, albedoImageSampler, nullptr);
		vkDestroyImageView(device->logicalDevice, albedoImageView, nullptr);
		vkDestroyImage(device->logicalDevice, albedoImage, nullptr);
		vkFreeMemory(device->logicalDevice, albedoImageMemory, nullptr);
		// Normal
		vkDestroySampler(device->logicalDevice, normalImageSampler, nullptr);
		vkDestroyImageView(device->logicalDevice, normalImageView, nullptr);
		vkDestroyImage(device->logicalDevice, normalImage, nullptr);
		vkFreeMemory(device->logicalDevice, normalImageMemory, nullptr);

		vkDestroyBuffer(device->logicalDevice, indexBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, indexBufferMemory, nullptr);
		vkDestroyBuffer(device->logicalDevice, vertexBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, vertexBufferMemory, nullptr);

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

