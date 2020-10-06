#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

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

	struct Vertex {
		glm::vec3 position;
		glm::vec3 color;

		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}
		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescription() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescription = {};
			attributeDescription[0].binding = 0;
			attributeDescription[0].location = 0;
			attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[0].offset = offsetof(Vertex, position);
			attributeDescription[1].binding = 0;
			attributeDescription[1].location = 1;
			attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[1].offset = offsetof(Vertex, color);
			return attributeDescription;
		}
	};

	const std::vector<Vertex> verts = {
		{{-0.5f, -0.5f, 0.0f}, {0.8f, 0.8f, 0.0f}},		//TL
		{{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}},		//TR
		{{ 0.5f,  0.5f, 0.0f}, {0.7f, 0.4f, 0.0f}},		//BR
		{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}}		//BL
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
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
	bool windowResized = false;

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
	std::vector<VkFramebuffer> swapchainFrameBuffers;
	VkExtent2D swapchainExtent;
	VkFormat swapchainFormat;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;


	const int MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageIsInFlight;

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
		CreateFrameBuffers();
		CreateCommandPool();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	void RecreateSwapchain()
	{
		std::cout << "Swapchain resized: " << windowResized << std::endl;

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
		CreateFrameBuffers();
		CreateCommandBuffers();
	}

	void CleanupSwapchain()
	{
		for (const auto& f : swapchainFrameBuffers)
			vkDestroyFramebuffer(device->logicalDevice, f, nullptr);

		vkFreeCommandBuffers(device->logicalDevice, commandPool, (uint32_t)commandBuffers.size(), commandBuffers.data());

		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
		vkDestroyRenderPass(device->logicalDevice, renderPass, nullptr);

		for (const auto& v : swapchainImageViews)
			vkDestroyImageView(device->logicalDevice, v, nullptr);
		//swapchainImageViews.clear();
		//swapchainImages.clear();

		//vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
		//vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);

		vkDestroySwapchainKHR(device->logicalDevice, swapchain, nullptr);
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			RenderFrame();
			glfwPollEvents();
		}

		vkDeviceWaitIdle(device->logicalDevice);
	}

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

	void Cleanup()
	{
		CleanupSwapchain();

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

		vkDestroyCommandPool(device->logicalDevice, commandPool, nullptr);

		device->Cleanup();

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

// ====================================
// INITIALIZATION
// ====================================

	void CreateWindow()
	{
		if (!glfwInit())
			throw std::runtime_error("Failed to init GLFW");
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(width, height, title, NULL, NULL);
		if (!window)
			throw std::runtime_error("Failed to create GLFW window");

		glfwRequiredExtentions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtentionsCount);
		for (uint32_t i = 0; i < glfwRequiredExtentionsCount; i++)
		{
			instanceExtensions.push_back(glfwRequiredExtentions[i]);
		}

		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
	}

	static void WindowResizeCallback(GLFWwindow* _window, int _width, int _height)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(_window));
		app->windowResized = true;
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
			int tmpWidth, tmpHeight;
			glfwGetFramebufferSize(window, &tmpWidth, &tmpHeight);
			VkExtent2D actual = {(uint32_t)tmpWidth, (uint32_t)tmpHeight};
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

	void CreateFrameBuffers()
	{
		swapchainFrameBuffers.resize((uint32_t)swapchainImages.size());

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.width = swapchainExtent.width;
		createInfo.height = swapchainExtent.height;
		createInfo.renderPass = renderPass;
		createInfo.layers = 1;
		createInfo.attachmentCount = 1;

		for (uint32_t i = 0; i < (uint32_t)swapchainImages.size(); i++)
		{
			VkImageView attachments[] = {swapchainImageViews[i]};
			createInfo.pAttachments = attachments;

			if (vkCreateFramebuffer(device->logicalDevice, &createInfo, nullptr, &swapchainFrameBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer");
		}
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
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference attachmentReference = {};
		attachmentReference.attachment = 0;
		attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentReference;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &attachmentDescription;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device->logicalDevice, &createInfo, nullptr, &renderPass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create renderpass");
	}

	void CreateGraphicsPipeline()
	{
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// ===== Shader Stage =====
		std::vector<char> vertFile = LoadFile(".\\res\\default_vert.spv");
		std::vector<char> fragFile = LoadFile(".\\res\\default_frag.spv");
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
		VkVertexInputBindingDescription vertInputBinding = Vertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 2> vertInputAttribute = Vertex::GetAttributeDescription();

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

		vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);
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

	void CreateCommandPool()
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = device->queueFamilyIndices.graphics;
		createInfo.flags = 0;

		if (vkCreateCommandPool(device->logicalDevice, &createInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create commandpool");
	}

	void CreateCommandBuffers()
	{
		commandBuffers.resize((uint32_t)swapchainFrameBuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device->logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to create commandbuffers");

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
			renderPassBeginInfo.renderArea.offset = {0, 0};
			renderPassBeginInfo.renderArea.extent = swapchainExtent;
			VkClearValue clearColor = {0.4f, 0.15f, 0.6f, 1.0f};
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			VkBuffer vertBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to record command buffer");
		}
	}

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

// Modularization can really happen here

	void CreateBuffer(VkDeviceSize _bufferSize, VkBufferUsageFlags _bufferUsage, VkMemoryPropertyFlags _memoryProperties, VkBuffer& _buffer, VkDeviceMemory& _memory)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = _bufferSize;
		bufferInfo.usage = _bufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device->logicalDevice, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to create buffer");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device->logicalDevice, _buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, _memoryProperties);

		// TODO : allocate more than 1 buffer's memory per AllocateMemory call
		if (vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &_memory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate vertex buffer memory");

		vkBindBufferMemory(device->logicalDevice, _buffer, _memory, 0);
	}

	void CreateVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(verts[0]) * (uint32_t)verts.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, verts.data(), (uint32_t)bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingBufferMemory);

		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			vertexBuffer,
			vertexBufferMemory
		);

		CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingBufferMemory, nullptr);
	}

	void CreateIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * (uint32_t)indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (uint32_t)bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingBufferMemory);

		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			indexBuffer,
			indexBufferMemory
		);

		CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingBufferMemory, nullptr);
	}

	// TODO : Create a transient command pool for one-shot buffers like this
	void CopyBuffer(VkBuffer _src, VkBuffer _dst, VkDeviceSize _size)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device->logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _size;

		vkCmdCopyBuffer(commandBuffer, _src, _dst, 1, &copyRegion);
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		// TODO : Use transfer queue & a fence (not queueWaitIdle)
		vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->graphicsQueue);

		vkFreeCommandBuffers(device->logicalDevice, commandPool, 1, &commandBuffer);
	}

	uint32_t FindMemoryType(uint32_t _filter, VkMemoryPropertyFlags _properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (_filter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
				return i;
		}

		throw std::runtime_error("Failed to find suitable memory type");
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
