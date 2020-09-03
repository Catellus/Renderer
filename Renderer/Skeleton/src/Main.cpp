#include <iostream>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "VulkanDevice.h"

class Application
{
private:
	const char* appTitle = "TestApp";
	const uint32_t appVersion = VK_MAKE_VERSION(0, 0, 0);
	const char* engineTitle = "TestEngine";
	const uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 0);

	uint32_t width = 800;
	uint32_t height = 600;
	const char* title = "Window Title";
	GLFWwindow* window;
	VkSurfaceKHR surface;

	std::vector<const char*> enabledLayers = {"VK_LAYER_KHRONOS_validation"};
	std::vector<const char*> enabledExtensions = {};
	const char** glfwRequiredExtentions;
	uint32_t glfwRequiredExtentionsCount;
	VkInstance instance;
	VulkanDevice* device;


public:
	~Application()
	{
		device->~VulkanDevice();
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
	}

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
			enabledExtensions.push_back(glfwRequiredExtentions[i]);
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
		createInfo.enabledLayerCount = (uint32_t)enabledLayers.size();
		createInfo.ppEnabledLayerNames = enabledLayers.data();
		createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();

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

		uint32_t selectedDevice = 0;

		// Select device
		//for (selectedDevice; selectedDevice < deviceCount; selectedDevice++)
		//{
		//}

		device = new VulkanDevice(instance, devices[selectedDevice]);
		device->queueFamilyIndices.graphics = device->FindQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		device->queueFamilyIndices.transfer = device->FindQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
		device->CreateLogicalDevice();
	}

};

int main()
{
	try
	{
		Application app;

		app.CreateWindow();
		app.CreateInstance();
		app.CreateSurface();

		app.CreateVulkanDevice();
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
