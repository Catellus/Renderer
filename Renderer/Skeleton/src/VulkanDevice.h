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
	std::vector<VkPhysicalDeviceGroupProperties> groupProperties;
	std::vector<VkExtensionProperties> extensionProperties;

	struct {
		uint32_t graphics;
		uint32_t transfer;
	} queueFamilyIndices;

// ===== Functions =====
	VulkanDevice(VkInstance _instance, VkPhysicalDevice _device)
	{
		physicalDevice = _device;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &features);

		uint32_t groupCount;
		vkEnumeratePhysicalDeviceGroups(_instance, &groupCount, nullptr);
		groupProperties.resize(groupCount);
		vkEnumeratePhysicalDeviceGroups(_instance, &groupCount, groupProperties.data());

		uint32_t extensionPropertyCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);
		extensionProperties.resize(extensionPropertyCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperties.data());
	}

	bool IsDeviceSuitable(VkPhysicalDevice _device)
	{
		return true;
	}

	void CreateLogicalDevice()
	{
		
	}
};


