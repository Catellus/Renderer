#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "VulkanDevice.h"

class Application
{
private:
// ===== Predefined =====
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

	uint32_t width = 800;
	uint32_t height = 600;
	const char* title = "Window Title";
	GLFWwindow* window;

// ===== Vulkan Members =====
	VkInstance instance;
	VulkanDevice* device;
	VkSurfaceKHR surface;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent;
	VkFormat swapchainFormat;

// ===== Shaders =====
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

// ====================================
public:
	void Run()
	{
		Initialize();
		MainLoop();
		Cleanup();
	}

	void Initialize()
	{
		CreateWindow();
		CreateInstance();
		CreateSurface();
		CreateVulkanDevice();
		CreateSwapchain();
		CreateRenderpass();
		CreateGraphicsPipeline();
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void Cleanup()
	{
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
		vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);

		vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);

		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);

		for (const auto& v : swapchainImageViews)
			vkDestroyImageView(device->logicalDevice, v, nullptr);
		swapchainImageViews.clear();
		swapchainImages.clear();

		vkDestroySwapchainKHR(device->logicalDevice, swapchain, nullptr);

		device->Cleanup();

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
	}

// ====================================
// INITIALIZATION
// ====================================

	void CreateWindow()
	{
		if (!glfwInit())
			throw std::runtime_error("Failed to init GLFW");
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(width, height, title, NULL, NULL);
		if (!window)
			throw std::runtime_error("Failed to create GLFW window");

		glfwRequiredExtentions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtentionsCount);
		for (uint32_t i = 0; i < glfwRequiredExtentionsCount; i++)
		{
			instanceExtensions.push_back(glfwRequiredExtentions[i]);
		}
	}

	void CreateSurface()
	{
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
	}

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

	void CreateVulkanDevice()
	{
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
			throw std::runtime_error("Failed to enumerate physical devices");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		uint32_t selectedDevice = ChooseSuitableDevice(devices);

		device = new VulkanDevice(instance, devices[selectedDevice]);
		device->queueFamilyIndices.graphics = device->FindQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		device->queueFamilyIndices.transfer = device->FindQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
		device->queueFamilyIndices.present = GetPresentFamilyIndex(device->queueProperties, device->physicalDevice);
		device->enabledExtensions = deviceExtensions;
		device->CreateLogicalDevice();
	}

	uint32_t ChooseSuitableDevice(std::vector<VkPhysicalDevice> _devices)
	{
		uint32_t supportedExtensionsCount;
		std::vector<VkExtensionProperties> supportedExtensions;
		uint32_t qPropCount;
		std::vector<VkQueueFamilyProperties> qProps;

		int graphics = -1;
		int transfer = -1;
		int present  = -1;

		uint32_t i = 0;

		for (const auto& d : _devices)
		{
			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
			vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, nullptr);
			supportedExtensions.resize(supportedExtensionsCount);
			vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, supportedExtensions.data());

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

			if (requiredExtensions.empty() && graphics >= 0 && transfer >= 0 && present >= 0 && GetSurfaceProperties(d).isSuitable())
				return i;

			i++;
		}

		throw std::runtime_error("Failed to find a suitable physical device");
		return 0;
	}

	uint32_t GetQueueFamilyIndex(std::vector<VkQueueFamilyProperties> _families, VkQueueFlags _flag)
	{
		uint32_t bestFit = -1;
		for (uint32_t i = 0; i < (uint32_t)_families.size(); i++)
		{
			if (_flag & _families[i].queueFlags)
			{
				if (_flag == VK_QUEUE_TRANSFER_BIT)
				{
					if ((_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
					{
						return i;
					}
					else
					{
						bestFit = i;
					}
				}
				else
				{
					return i;
				}
			}
		}

		if (bestFit == -1)
			throw std::runtime_error("Failed to find a queue index");
		return bestFit;
	}

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

		for (uint32_t i = 0; i < imageCount; i++)
		{
			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.image = swapchainImages[i];
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = format.format;
			viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.layerCount     = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.levelCount   = 1;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;

			if (vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create swapchain image view");

			swapchainExtent = extent;
			swapchainFormat = format.format;
		}
	}

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
			VkExtent2D actual = { width, height };
			actual.width = clamp(actual.width, _properties.capabilities.minImageExtent.width, _properties.capabilities.maxImageExtent.width);
			actual.height = clamp(actual.height, _properties.capabilities.minImageExtent.height, _properties.capabilities.maxImageExtent.height);
			_extent = actual;
		}
	}

	uint32_t clamp(uint32_t _val, uint32_t _min, uint32_t _max)
	{
		if (_val < _min)
			return _min;
		if (_val > _max)
			return _max;
		return _val;
	}

	void CreateBuffers()
	{

	}

	void CreateRenderpass()
	{
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.format = swapchainFormat;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkAttachmentReference attachmentReference = {};
		attachmentReference.attachment = 0;
		attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentReference;

		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &attachmentDescription;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(device->logicalDevice, &createInfo, nullptr, &renderPass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create renderpass");
	}

	void CreateGraphicsPipeline()
	{
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// ===== Shader Stage =====
        std::vector<char> vertFile = LoadFile("C:\\dev\\Renderer_REPO\\Renderer\\Skeleton\\res\\vert.spv");
        std::vector<char> fragFile = LoadFile("C:\\dev\\Renderer_REPO\\Renderer\\Skeleton\\res\\frag.spv");
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

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;

	// ===== Vertex Input =====
		VkVertexInputBindingDescription vertInputBinding = {};
		vertInputBinding.binding = 0;
		vertInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertInputBinding.stride = 3 * sizeof(float);
		VkVertexInputAttributeDescription vertInputAttribute = {};
		vertInputAttribute.binding = 0;
		vertInputAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		vertInputAttribute.location = 0;
		vertInputAttribute.offset = 0;

		VkPipelineVertexInputStateCreateInfo vertInputState = {};
		vertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertInputState.vertexBindingDescriptionCount = 1;
		vertInputState.pVertexBindingDescriptions = &vertInputBinding;
		vertInputState.vertexAttributeDescriptionCount = 1;
		vertInputState.pVertexAttributeDescriptions = &vertInputAttribute;

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
		scissor.offset = {0, 0};

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
		rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;

		pipelineCreateInfo.pDepthStencilState = nullptr;

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
	}

	void CreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		if (vkCreatePipelineLayout(device->logicalDevice, &createInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline layout");
	}

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

	std::vector<char> LoadFile(const char* _directory)
	{
		std::ifstream stream;
		stream.open(_directory, std::ios::ate | std::ios::binary);
		if (!stream.is_open())
			throw std::runtime_error("Failed to open shader directory");

		uint32_t size = (uint32_t) stream.tellg();
		stream.seekg(0);

		std::vector<char> bytes(size);
		stream.read(bytes.data(), size);

		stream.close();
		return bytes;
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
