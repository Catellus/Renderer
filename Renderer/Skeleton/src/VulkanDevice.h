#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

struct VulkanDevice
{
// ==============================================
// Variables
// ==============================================
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceFeatures enabledFeatures;
	std::vector<const char*> enabledExtensions;
	std::vector<VkQueueFamilyProperties> queueProperties;
	std::vector<VkExtensionProperties> extensionProperties;

	std::vector<VkCommandPool> commandPools;
	uint32_t transientPoolIndex;
	uint32_t graphicsCommandPoolIndex;

	struct {
		uint32_t graphics;
		uint32_t transfer;
		uint32_t present;
	} queueFamilyIndices;
	VkQueue graphicsQueue;
	VkQueue transferQueue;
	VkQueue presentQueue;

// ==============================================
// Initialization
// ==============================================

	// Gathers information about the GPU
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

	// Defines and creates a logical device with queues
	void CreateLogicalDevice()
	{
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();

		enabledFeatures.samplerAnisotropy = VK_TRUE;
		createInfo.pEnabledFeatures = &enabledFeatures;

		// Define the queues to create
		uint32_t i = 0;
		const float priority = 1.0f;
		const uint32_t queueIndices[] = { queueFamilyIndices.graphics, queueFamilyIndices.transfer, queueFamilyIndices.present };
		std::printf("graphics: %d, transfer: %d, presentation: %d\n", queueFamilyIndices.graphics, queueFamilyIndices.transfer, queueFamilyIndices.present);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(3);
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
		vkGetDeviceQueue(logicalDevice, queueFamilyIndices.present, 0, &presentQueue);

		transientPoolIndex = CreateCommandPool(queueFamilyIndices.transfer, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	}

// ==============================================
// Cleanup
// ==============================================

	// Destroys this logical device & its command pools
	void Cleanup()
	{
		for (const auto& pool : commandPools)
			vkDestroyCommandPool(logicalDevice, pool, nullptr);

		if (logicalDevice)
		{
			vkDestroyDevice(logicalDevice, nullptr);
		}

		delete(this);
	}

// ==============================================
// Runtime
// ==============================================

	// Returns the index of the first queue family that meets the input requirements
	int FindQueueFamilyIndex(VkQueueFlagBits _flag)
	{
		int bestFit = -1;
		for (uint32_t i = 0; i < (uint32_t)queueProperties.size(); i++)
		{
			if (_flag & queueProperties[i].queueFlags)
			{
				if (_flag == VK_QUEUE_TRANSFER_BIT)
				{
					if ((queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
						return i;
					else
						bestFit = i;
				}
				else
					return i;
			}
		}

		if (bestFit == -1)
			throw std::runtime_error("Failed to find a queue index");
		return bestFit;
	}

	// Returns a command pool in the given queue family
	uint32_t CreateCommandPool(uint32_t _queue, VkCommandPoolCreateFlags _flags)
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = _queue;
		createInfo.flags = _flags;

		VkCommandPool tmpPool;
		if (vkCreateCommandPool(logicalDevice, &createInfo, nullptr, &tmpPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool");

		commandPools.push_back(tmpPool);
		return (uint32_t)commandPools.size() - 1;
	}

	// Creates a buffer of the given properties, and allocates and binds its memory
	void CreateBuffer(VkDeviceSize _bufferSize, VkBufferUsageFlags _bufferUsage, VkMemoryPropertyFlags _memoryProperties, VkBuffer& _buffer, VkDeviceMemory& _memory)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = _bufferSize;
		bufferInfo.usage = _bufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to create buffer");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, _buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, _memoryProperties);

		// TODO : allocate more than 1 buffer's memory per AllocateMemory call
		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &_memory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate vertex buffer memory");

		vkBindBufferMemory(logicalDevice, _buffer, _memory, 0);
	}

	uint32_t FindMemoryType(uint32_t _filter, VkMemoryPropertyFlags _properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (_filter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
				return i;
		}

		throw std::runtime_error("Failed to find suitable memory type");
	}

	void CopyBuffer(VkBuffer _src, VkBuffer _dst, VkDeviceSize _size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(transientPoolIndex);

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _size;
		vkCmdCopyBuffer(commandBuffer, _src, _dst, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer, transientPoolIndex, transferQueue);
	}

	// Maps buffer memory and copies input data to it
	void CopyDataToBufferMemory(const void* _srcData, VkDeviceSize _size, VkDeviceMemory& _memory, VkDeviceSize _offset = 0, VkMemoryMapFlags _flags = 0)
	{
		void* tmpData;
		vkMapMemory(logicalDevice, _memory, _offset, _size, _flags, &tmpData);
		memcpy(tmpData, _srcData, _size);
		vkUnmapMemory(logicalDevice, _memory);
	}

	// Create and begin recording a single use command
	VkCommandBuffer BeginSingleTimeCommands(uint32_t _poolIndex)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPools[_poolIndex];
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// Begin recording commands
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	// Finish recording and execute a single use command
	void EndSingleTimeCommands(VkCommandBuffer& _commandBuffer, uint32_t _poolIndex, VkQueue _queue)
	{
		// Finish recording commands
		vkEndCommandBuffer(_commandBuffer);

		// Execute recorded command
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_commandBuffer;

// TODO : Use transfer queue & a fence (not queueWaitIdle)
		vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(_queue);
		// Destroy command
		vkFreeCommandBuffers(logicalDevice, commandPools[_poolIndex], 1, &_commandBuffer);
	}

	// Creates a buffer in GPU memory for the input data
	// Copies the input data into the buffer with a staging buffer
	void CreateAndFillBuffer(const void* _data, VkDeviceSize _size, VkBuffer& _buffer, VkDeviceMemory& _memory, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memProps)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		CopyDataToBufferMemory(_data, _size, stagingBufferMemory);

		CreateBuffer(
			_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | _usage,
			_memProps,
			_buffer,
			_memory
		);

		CopyBuffer(stagingBuffer, _buffer, _size);

		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}

};


