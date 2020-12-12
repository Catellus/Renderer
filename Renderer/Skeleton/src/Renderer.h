#pragma once

#include <iostream>
#include <vector>

#include "Common.h"
#include "VulkanDevice.h"
#include "Object.h"
#include "Shaders.h"
#include "Mesh.h"
#include "Texture.h"
#include "Initializers.h"

#define CheckResultCritical(x, message)			\
	VkResult vkFunctionResult = x;				\
	if (vkFunctionResult != VK_SUCCESS)			\
	{											\
		assert(vkFunctionResult != VK_SUCCESS);	\
		throw std::runtime_error(message);		\
	}

namespace skel {

class Renderer
{
public:

// ------------------------------------------- //
// Member structs
// ------------------------------------------- //

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

	struct PipelineInformation
	{
		VkRenderPass renderPass;
		VkPipelineLayout layout;
		VkPipeline pipeline;
	};

// ------------------------------------------- //
// Member variables
// ------------------------------------------- //

	// Vulkan instance creation properties
	std::vector<const char*> instanceLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> instanceExtensions = {};
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const char** glfwRequiredExtentions;
	uint32_t glfwRequiredExtentionsCount;

	std::vector<std::vector<Object*>*>* renderableObjects;

// ------------------------------------------- //
// Renderer
// ------------------------------------------- //

	GLFWwindow* window;
	bool windowResized;

	VkInstance instance;
	VkSurfaceKHR surface;
	VulkanDevice* device;
	uint32_t graphicsCommandPoolIndex;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFrameBuffers;
	VkExtent2D swapchainExtent;
	VkFormat swapchainFormat;

	VkImage depthImage;
	VkImageView depthImageView;
	VkDeviceMemory depthImageMemory;

	std::vector<skel::shaders::ShaderDescriptorInformation*> shaderDescriptors;
	VkRenderPass renderpass;
	std::vector<VkPipelineLayout> pipelineLayouts;
	std::vector<VkPipeline> pipelines;

	// Synchronization
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageIsInFlight;

	std::vector<VkCommandBuffer> commandBuffers;

public:
	Renderer(GLFWwindow*);
	~Renderer();

	// Creates the render pipeline & components
	void Initialize();

	// Renderer updating
	// ==========================================

	void RecreateRenderer();
	void CreateRenderer();
	void CleanupRenderer();

	// Handles rendering and presentation to the window
	void RenderFrame();

	// Basic initialization
	// ==========================================

	// Retrieve all extensions GLFW needs
	void GetGLFWRequiredExtensions();
	// Creates a Vulkan instance with app information
	void CreateInstance();
	// Make GLFW create a surface to present images to
	void CreateSurface();
	// Selects a physical device and initializes a logical device
	void CreateVulkanDevice();
	// Determines the suitability of physical devices and selects the first suitable
	uint32_t ChooseSuitableDevice(std::vector<VkPhysicalDevice>, uint32_t&, uint32_t&, uint32_t&);
	// Returns the index of the first queue with desired properties
	uint32_t GetQueueFamilyIndex(std::vector<VkQueueFamilyProperties>, VkQueueFlags);
	// Returns the index of the first queue that supports the surface -- Queries for WSI support
	int GetPresentFamilyIndex(std::vector<VkQueueFamilyProperties>, VkPhysicalDevice);
	// Returns the device's supported image and presentation information
	SurfaceProperties GetSurfaceProperties(VkPhysicalDevice);

	// Creates a set of semaphores and a fence for every frame
	void CreateSyncObjects();
	// Create the basic command pool for rendering
	void CreateCommandPools();

	// Renderer creation
	// ==========================================

	// Creates a swapchain and its images as rendering canvases
	void CreateSwapchain();
	// Finds a surface with ideal format, present mode, and extent properties
	void GetIdealSurfaceProperties(SurfaceProperties, VkSurfaceFormatKHR&, VkPresentModeKHR&, VkExtent2D&);
	// Creates a compact image view object
	VkImageView CreateImageView(VulkanDevice*, VkImage, VkFormat, VkImageAspectFlags);
	// Specify what types of attachments will be accessed by the graphics pipeline
	void CreateRenderPass();
	// Returns the first format of the desired properties supported by the GPU
	VkFormat FindSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);
	// Binds shader uniforms
	void CreatePipelineLayout(VkPipelineLayout&, VkDescriptorSetLayout*, uint32_t = 1);
	// Define the properties for each stage of the graphics pipeline
	void CreateGraphicsPipeline(const char*, const char*, const VkPipelineLayout&, VkPipeline&);
	// Create a pipeline usable object for shader code
	VkShaderModule CreateShaderModule(const std::vector<char>);
	// Creates an image and view for the depth texture
	void CreateDepthResources();
	// Specifies the image view(s) to bind to renderpass attachments
	void CreateFrameBuffers();
	// Allocates space for, creates, and returns a set of rendering command buffers
	void CreateAndBeginCommandBuffers();
	// Records draw commands into the command buffers
	void RecordRenderingCommandBuffers(std::vector<std::vector<Object*>*>* = nullptr);
	// Allocate memory for command recording
	uint32_t AllocateCommandBuffers(VkCommandBufferLevel, uint32_t, std::vector<VkCommandBuffer>&);
	void EndCommandBuffer(VkCommandBuffer&);

	// Helpers
	// ==========================================

	// Returns the depth texture's image format
	VkFormat FindDepthFormat();
	// Loads a file as binary -- Returns a character vector
	std::vector<char> LoadFile(const char*);

	void AddShader( const char*, const std::vector<VkDescriptorSetLayoutBinding>&, uint32_t);

}; // Renderer

} // skel

