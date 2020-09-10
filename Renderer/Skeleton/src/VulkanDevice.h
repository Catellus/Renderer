#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

struct VulkanDevice
{
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceFeatures enabledFeatures;
	std::vector<const char*> enabledExtensions;
	std::vector<VkQueueFamilyProperties> queueProperties;
	std::vector<VkExtensionProperties> extensionProperties;

	struct {
		uint32_t graphics;
		uint32_t transfer;
	} queueFamilyIndices;
	VkQueue graphicsQueue;
	VkQueue transferQueue;

// ===== Functions =====
	VulkanDevice(VkInstance _instance, VkPhysicalDevice _device)
	{
		physicalDevice = _device;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &features);
		enabledFeatures = {};
		std::cout << properties.deviceName << std::endl;

		uint32_t queuesCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuesCount, nullptr);
		queueProperties.resize(queuesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuesCount, queueProperties.data());

		uint32_t extensionPropertyCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);
		extensionProperties.resize(extensionPropertyCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperties.data());
	}

	~VulkanDevice()
	{
		if (logicalDevice)
		{
			vkDestroyDevice(logicalDevice, nullptr);
		}
	}

	uint32_t FindQueueFamilyIndex(VkQueueFlagBits _flag)
	{
		uint32_t bestFit = -1;
		for (uint32_t i = 0; i < (uint32_t)queueProperties.size(); i++)
		{
			if (_flag & queueProperties[i].queueFlags)
			{
				if (_flag == VK_QUEUE_TRANSFER_BIT)
				{
					if ((queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
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

	void CreateLogicalDevice()
	{
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.enabledExtensionCount   = (uint32_t)enabledExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		createInfo.pEnabledFeatures = &enabledFeatures;

		uint32_t i = 0;
		const float priority = 1.0f;
		const uint32_t queueIndices[] = {queueFamilyIndices.graphics, queueFamilyIndices.transfer};
		std::printf("%d, %d\n", queueFamilyIndices.graphics, queueFamilyIndices.transfer);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(2);
		for (auto& ci : queueCreateInfos)
		{
			ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			ci.pQueuePriorities = &priority;
			ci.queueCount = 1;
			ci.queueFamilyIndex = queueIndices[i++];
		}
		createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device");

		vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphics, 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, queueFamilyIndices.transfer, 0, &transferQueue);
	}
};


