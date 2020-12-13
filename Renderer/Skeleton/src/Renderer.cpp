
#include "Renderer.h"

#include <set>
#include <string>
#include <fstream>

skel::Renderer::Renderer(GLFWwindow* _window)
{
	pipelineLayouts.resize(2);
	pipelines.resize(2);

	window = _window;
	GetGLFWRequiredExtensions();

	CreateInstance();
	CreateSurface();
	CreateVulkanDevice();
	CreateCommandPools();
}

skel::Renderer::~Renderer()
{
	vkFreeCommandBuffers(
		device->logicalDevice,
		device->commandPools[graphicsCommandPoolIndex],
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data()
	);

	CleanupRenderer();

	for (const auto& descriptor : shaderDescriptors)
	{
		descriptor->Cleanup(device->logicalDevice);
		free(descriptor);
	}

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device->logicalDevice, renderCompleteSemaphores[i], nullptr);
		vkDestroySemaphore(device->logicalDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device->logicalDevice, inFlightFences[i], nullptr);
	}

	device->Cleanup();
	free(device);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}

// Creates the render pipeline & components
void skel::Renderer::Initialize()
{
	CreateRenderer();
	CreateSyncObjects();
}

void skel::Renderer::RecreateRenderer()
{
	vkDeviceWaitIdle(device->logicalDevice);

	// Don't recreate renderer when minimized
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	// Destroy all renderer objects
	CleanupRenderer();
	// Create Renderer
	CreateRenderer();
}

void skel::Renderer::CreateRenderer()
{
	CreateSwapchain();
	CreateRenderPass();

	std::string shaderDirectory = std::string(shaderPrefix);

	for (uint32_t i = 0; i < static_cast<uint32_t>(shaderDescriptors.size()); i++)
	{
		CreatePipelineLayout(pipelineLayouts[i], &shaderDescriptors[i]->descriptorSetLayout);
		CreateGraphicsPipeline(
			(shaderDirectory + shaderDescriptors[i]->shaderName + "_vert.spv").c_str(),
			(shaderDirectory + shaderDescriptors[i]->shaderName + "_frag.spv").c_str(),
			pipelineLayouts[i],
			pipelines[i]
			);
	}

	CreateDepthResources();
	CreateFrameBuffers();
	CreateAndBeginCommandBuffers();
	RecordRenderingCommandBuffers();
}

void skel::Renderer::CleanupRenderer()
{
	vkDestroyImageView(device->logicalDevice, depthImageView, nullptr);
	vkDestroyImage(device->logicalDevice, depthImage, nullptr);
	vkFreeMemory(device->logicalDevice, depthImageMemory, nullptr);

	for (const auto& f : swapchainFrameBuffers)
		vkDestroyFramebuffer(device->logicalDevice, f, nullptr);

	for (uint32_t i = 0; i < static_cast<uint32_t>(pipelines.size()); i++)
	{
		vkDestroyPipeline(device->logicalDevice, pipelines[i], nullptr);
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayouts[i], nullptr);
	}
	vkDestroyRenderPass(device->logicalDevice, renderpass, nullptr);

	for (const auto& v : swapchainImageViews)
		vkDestroyImageView(device->logicalDevice, v, nullptr);

	vkDestroySwapchainKHR(device->logicalDevice, swapchain, nullptr);
}

// Handles rendering and presentation to the window
void skel::Renderer::RenderFrame()
{
	uint32_t imageIndex;

	// Wait for and retrieve the next frame for rendering
	vkWaitForFences(device->logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(
		device->logicalDevice,
		swapchain,
		UINT64_MAX,
		imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE,
		&imageIndex
		);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateRenderer();
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
	CheckResultCritical(
		vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]),
		"Failed to submit draw command buffer"
		);

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
		RecreateRenderer();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swapchain");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ==============================================
// Initialization
// ==============================================

// Retrieve all extensions GLFW needs
void skel::Renderer::GetGLFWRequiredExtensions()
{
	glfwRequiredExtentions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtentionsCount);
	for (uint32_t i = 0; i < glfwRequiredExtentionsCount; i++)
	{
		instanceExtensions.push_back(glfwRequiredExtentions[i]);
	}
}

// Creates a Vulkan instance with app information
void skel::Renderer::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Temp app title";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = "Temp engine name";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
	createInfo.ppEnabledLayerNames = instanceLayers.data();
	createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	CheckResultCritical(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create vkInstance");
}

// Make GLFW create a surface to present images to
void skel::Renderer::CreateSurface()
{
	glfwCreateWindowSurface(instance, window, nullptr, &surface);
}

// Selects a physical device and initializes a logical device
void skel::Renderer::CreateVulkanDevice()
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

// Determines the suitability of physical devices and selects the first suitable
uint32_t skel::Renderer::ChooseSuitableDevice(
	std::vector<VkPhysicalDevice> _devices,
	uint32_t& _graphicsIndex,
	uint32_t& _transferIndex,
	uint32_t& _presentIndex
	)
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
		// Check for the presence of all required extensions
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, nullptr);
		supportedExtensions.resize(supportedExtensionsCount);
		vkEnumerateDeviceExtensionProperties(d, nullptr, &supportedExtensionsCount, supportedExtensions.data());

		vkGetPhysicalDeviceFeatures(d, &supportedFeatures);

		for (const auto& e : supportedExtensions)
		{
			requiredExtensions.erase(e.extensionName);
		}

		// Find required queue family indices
		vkGetPhysicalDeviceQueueFamilyProperties(d, &qPropCount, nullptr);
		qProps.resize(qPropCount);
		vkGetPhysicalDeviceQueueFamilyProperties(d, &qPropCount, qProps.data());

		graphics = GetQueueFamilyIndex(qProps, VK_QUEUE_GRAPHICS_BIT);
		transfer = GetQueueFamilyIndex(qProps, VK_QUEUE_TRANSFER_BIT);
		present = GetPresentFamilyIndex(qProps, d);

		// If all required elements are present, use this device
		if (requiredExtensions.empty()
			&& graphics >= 0
			&& transfer >= 0
			&& present >= 0
			&& GetSurfaceProperties(d).isSuitable()
			&& supportedFeatures.samplerAnisotropy
			)
		{
			_graphicsIndex = graphics;
			_transferIndex = transfer;
			_presentIndex = present;
			return i;
		}

		i++;
	}

	throw std::runtime_error("Failed to find a suitable physical device");
	return 0;
}

// Returns the index of the first queue with desired properties
uint32_t skel::Renderer::GetQueueFamilyIndex(std::vector<VkQueueFamilyProperties> _families, VkQueueFlags _flag)
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

// Returns the index of the first queue that supports the surface -- Queries for WSI support
int skel::Renderer::GetPresentFamilyIndex(std::vector<VkQueueFamilyProperties> qProps, VkPhysicalDevice d)
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
skel::Renderer::SurfaceProperties skel::Renderer::GetSurfaceProperties(VkPhysicalDevice _device)
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

// Creates a set of semaphores and a fence for every frame
void skel::Renderer::CreateSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imageIsInFlight.resize(static_cast<uint32_t>(swapchainImages.size()), VK_NULL_HANDLE);

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

// Create the basic command pool for rendering
void skel::Renderer::CreateCommandPools()
{
	graphicsCommandPoolIndex = device->CreateCommandPool(device->queueFamilyIndices.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	device->graphicsCommandPoolIndex = graphicsCommandPoolIndex;
}

// Creates a swapchain and its images as rendering canvases
void skel::Renderer::CreateSwapchain()
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
	VkSwapchainCreateInfoKHR createInfo =
		skel::initializers::SwapchainCreateInfo(
			format,
			presentMode,
			extent,
			surface,
			imageCount
		);

	// TODO : Use only exclusive sharing & handle hand-offs manually (using pipeline barriers)
	if (device->queueFamilyIndices.graphics == device->queueFamilyIndices.present)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	else
	{
		uint32_t sharingIndices[] = { device->queueFamilyIndices.graphics, device->queueFamilyIndices.present };

		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = sharingIndices;
	}

	CheckResultCritical(
		vkCreateSwapchainKHR(device->logicalDevice, &createInfo, nullptr, &swapchain),
		"Failed to create swapchain"
		);

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
		swapchainImageViews[i] = CreateImageView(device, swapchainImages[i], swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

// Finds a surface with ideal format, present mode, and extent properties
void skel::Renderer::GetIdealSurfaceProperties(
	SurfaceProperties _properties,
	VkSurfaceFormatKHR& _format,
	VkPresentModeKHR& _presentMode,
	VkExtent2D& _extent
	)
{
	// Search for SRGB and 32 bit color capabilities
	_format = _properties.formats[0];
	for (const auto& f : _properties.formats)
	{
		// Finds a color space using SRGB gamma correction -- (Use VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT for linear)
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

// Creates a compact image view object
VkImageView skel::Renderer::CreateImageView(VulkanDevice* _device, VkImage _image, VkFormat _format, VkImageAspectFlags _aspectFlags)
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
	CheckResultCritical(
		vkCreateImageView(_device->logicalDevice, &viewInfo, nullptr, &imageView),
		"Failed to create image view"
		);

	return imageView;
}

// Specify what types of attachments will be accessed by the graphics pipeline
void skel::Renderer::CreateRenderPass()
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

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 2;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	CheckResultCritical(
		vkCreateRenderPass(device->logicalDevice, &createInfo, nullptr, &renderpass),
		"Failed to create renderpass"
		);
}

// Returns the first format of the desired properties supported by the GPU
VkFormat skel::Renderer::FindSupportedFormat(const std::vector<VkFormat>& _candidates, VkImageTiling _tiling, VkFormatFeatureFlags _features)
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

// Binds shader uniforms
void skel::Renderer::CreatePipelineLayout(VkPipelineLayout& _pipelineLayout, VkDescriptorSetLayout* _shaderLayouts, uint32_t _count /*= 1*/)
{
	VkPipelineLayoutCreateInfo createInfo =
		skel::initializers::PipelineLayoutCreateInfo(
			_shaderLayouts,
			_count
		);

	CheckResultCritical(
		vkCreatePipelineLayout(device->logicalDevice, &createInfo, nullptr, &_pipelineLayout),
		"Failed to create graphics pipeline layout"
		);
}

// Define the properties for each stage of the graphics pipeline
void skel::Renderer::CreateGraphicsPipeline(
	const char* _vertShaderDir,
	const char* _fragShaderDir,
	const VkPipelineLayout& _pipelineLayout,
	VkPipeline& _pipeline
	)
{
// ===== Viewport =====
	VkViewport viewport = {};
	viewport.width = (float)swapchainExtent.width;
	viewport.height = (float)swapchainExtent.height;
	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
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
	//Vertex information (Position, Color, UV, etc)
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

// ===== Pipeline Creation =====
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = 
		skel::initializers::GraphicsPipelineCreateInfo(
			pipelineLayouts[0],
			renderpass
		);

	pipelineCreateInfo.pViewportState		= &viewportState;
	pipelineCreateInfo.pVertexInputState	= &vertInputState;
	pipelineCreateInfo.pInputAssemblyState	= &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState	= &rasterizationState;
	pipelineCreateInfo.pMultisampleState	= &mutisampleState;
	pipelineCreateInfo.pDepthStencilState	= &depthStencilState;
	pipelineCreateInfo.pColorBlendState		= &colorBlendState;
	pipelineCreateInfo.pDynamicState		= &dynamicState;

	std::vector<char> vertShaderFile = LoadFile(_vertShaderDir);
	std::vector<char> fragShaderFile = LoadFile(_fragShaderDir);
	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderFile);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderFile);

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

	VkPipelineShaderStageCreateInfo shaderStages[2] = {
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	// Create the unlit-shadered pipeline =====
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.layout = _pipelineLayout;

	CheckResultCritical(
		vkCreateGraphicsPipelines(device->logicalDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &_pipeline),
		"Failed to create unlit pipeline"
		);

	vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);
}

// Create a pipeline usable object for shader code
VkShaderModule skel::Renderer::CreateShaderModule(const std::vector<char> _code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = (uint32_t)_code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(_code.data());

	VkShaderModule tmpShaderModule;
	CheckResultCritical(
		vkCreateShaderModule(device->logicalDevice, &createInfo, nullptr, &tmpShaderModule),
		"Failed to create shader module"
		);

	return tmpShaderModule;
}

// Creates an image and view for the depth texture
void skel::Renderer::CreateDepthResources()
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

// Specifies the image view(s) to bind to renderpass attachments
void skel::Renderer::CreateFrameBuffers()
{
	swapchainFrameBuffers.resize((uint32_t)swapchainImages.size());

	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.width = swapchainExtent.width;
	createInfo.height = swapchainExtent.height;
	createInfo.renderPass = renderpass;
	createInfo.layers = 1;

	for (uint32_t i = 0; i < (uint32_t)swapchainImages.size(); i++)
	{
		std::array<VkImageView, 2> attachments = { swapchainImageViews[i], depthImageView };
		createInfo.attachmentCount = (uint32_t)attachments.size();
		createInfo.pAttachments = attachments.data();

		CheckResultCritical(
			vkCreateFramebuffer(device->logicalDevice, &createInfo, nullptr, &swapchainFrameBuffers[i]),
			"Failed to create framebuffer"
			);
	}
}

// Allocates space for a set of rendering command buffers
void skel::Renderer::CreateAndBeginCommandBuffers()
{
	AllocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, graphicsCommandPoolIndex, commandBuffers);
}

void skel::Renderer::RecordRenderingCommandBuffers(std::vector<std::vector<Object*>*>* _renderableObjects /*= nullptr*/)
{
	if (_renderableObjects != nullptr)
	{
		renderableObjects = _renderableObjects;
	}
	else if (renderableObjects == nullptr)
	{
		return;
	}

	vkDeviceWaitIdle(device->logicalDevice);
	vkResetCommandPool(device->logicalDevice, device->commandPools[graphicsCommandPoolIndex], 0);

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.02f, 0.025f, 0.03f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pInheritanceInfo = nullptr;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderpass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = swapchainExtent;
	renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();

	uint32_t commandCount = (uint32_t)swapchainFrameBuffers.size();

	for (uint32_t i = 0; i < commandCount; i++)
	{
		// Begin recording a command
		CheckResultCritical(
			vkBeginCommandBuffer(commandBuffers[i], &beginInfo),
			"Failed to begin command buffer"
		);

		renderPassBeginInfo.framebuffer = swapchainFrameBuffers[i];
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		for (uint32_t j = 0; j < static_cast<uint32_t>(renderableObjects->size()); j++)
		{
			std::vector<Object*>* objectGeneration = (*renderableObjects)[j];
			if (static_cast<uint32_t>(objectGeneration->size()) == 0)
				continue;

			uint32_t generationShaderType = (*objectGeneration)[0]->shader.type;

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[generationShaderType]);

			for (auto& obj : *objectGeneration)
			{
				obj->Draw(commandBuffers[i], pipelineLayouts[generationShaderType]);
			}
		}

		// Complete the recording
		EndCommandBuffer(commandBuffers[i]);
	}
}

// Allocate memory for command recording
uint32_t skel::Renderer::AllocateCommandBuffers(VkCommandBufferLevel _level, uint32_t _commandPoolIndex, std::vector<VkCommandBuffer>& _buffers)
{
	uint32_t size = (uint32_t)swapchainFrameBuffers.size();
	_buffers.resize(size);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device->commandPools[_commandPoolIndex];
	allocInfo.level = _level;
	allocInfo.commandBufferCount = size;

	CheckResultCritical(
		vkAllocateCommandBuffers(device->logicalDevice, &allocInfo, _buffers.data()),
		"Failed ot create command buffers"
		);

	return size;
}

void skel::Renderer::EndCommandBuffer(VkCommandBuffer& _buffer)
{
	vkCmdEndRenderPass(_buffer);

	CheckResultCritical(
		vkEndCommandBuffer(_buffer),
		"Failed to record command buffer"
		);
}

// ==============================================
// Helpers
// ==============================================

// Returns the depth texture's image format
VkFormat skel::Renderer::FindDepthFormat()
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

// Loads a file as binary -- Returns a character vector
std::vector<char> skel::Renderer::LoadFile(const char* _directory)
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

void skel::Renderer::AddShader(
	const char* _name,
	const std::vector<VkDescriptorSetLayoutBinding>& _bindings,
	uint32_t _objectCount
	)
{
	shaderDescriptors.push_back(new skel::shaders::ShaderDescriptorInformation(_name));
	skel::shaders::ShaderDescriptorInformation& newShader = *shaderDescriptors[static_cast<uint32_t>(shaderDescriptors.size()) - 1];
	newShader.CreateLayoutBindingsAndPool(device->logicalDevice, _bindings, _objectCount);
}

