#pragma  once

#include <iostream>
#include <string>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <vulkan/vulkan.h>

#include "VulkanDevice.h"
#include "Initializers.h"

namespace skel
{
	// Transforms the input image's layout for copying data into
	void TransitionImageLayout(VulkanDevice* _device, VkImage _image, VkFormat _format, VkImageLayout _oldLayout, VkImageLayout _newLayout)
	{
		VkCommandBuffer commandBuffer = _device->BeginSingleTimeCommands(_device->graphicsCommandPoolIndex);

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
		_device->EndSingleTimeCommands(commandBuffer, _device->graphicsCommandPoolIndex, _device->graphicsQueue);
	}

	void CreateTextureSampler(VulkanDevice* _device, VkSampler& _imageSampler)
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

		if (vkCreateSampler(_device->logicalDevice, &createInfo, nullptr, &_imageSampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler");
	}

	VkImageView CreateImageView(VulkanDevice* _device, VkImage _image, VkFormat _format, VkImageAspectFlags _aspectFlags)
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
		if (vkCreateImageView(_device->logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view");

		return imageView;
	}

	void CreateTextureImageView(VulkanDevice* _device, VkImage& _image, VkImageView& _imageView)
	{
		_imageView = CreateImageView(_device, _image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	// Defines and executes a command to copy the buffer to the image
	void CopyBufferToImage(VulkanDevice* _device, VkBuffer _buffer, VkImage _image, uint32_t _width, uint32_t _height)
	{
		VkCommandBuffer commandBuffer = _device->BeginSingleTimeCommands(_device->transientPoolIndex);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { _width, _height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, _buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		_device->EndSingleTimeCommands(commandBuffer, _device->transientPoolIndex, _device->transferQueue);
	}

	// Returns the index of the first memory type on the GPU with the desired filter and properties
	uint32_t FindMemoryType(VulkanDevice* _device, uint32_t _typeFilter, VkMemoryPropertyFlags _properties)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(_device->physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((_typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
				return i;
		}

		throw std::runtime_error("Failed to find suitable memory type");
	}

	// Creates an image, and allocates and binds memory for it
	void CreateImage(VulkanDevice* _device, uint32_t _width, uint32_t _height, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageMemory)
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

		if (vkCreateImage(_device->logicalDevice, &createInfo, nullptr, &_image) != VK_SUCCESS)
			throw std::runtime_error("Failed to crate image");

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(_device->logicalDevice, _image, &memoryRequirements);
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(_device, memoryRequirements.memoryTypeBits, _properties);

		if (vkAllocateMemory(_device->logicalDevice, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate image memory");

		vkBindImageMemory(_device->logicalDevice, _image, _imageMemory, 0);
	}

	// Loads the input texture, and copies it to an image
	void CreateTextureImage(VulkanDevice* _device, const std::string _directory, VkImage& _image, VkDeviceMemory& _imageMemory)
	{
		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = stbi_load(_directory.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = (uint64_t)textureWidth * (uint64_t)textureHeight * 4;

		if (!pixels)
			throw std::runtime_error("Failed to load texture image");

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		_device->CreateBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		_device->CopyDataToBufferMemory(pixels, (size_t)imageSize, stagingBufferMemory);
		stbi_image_free(pixels);

		CreateImage(
			_device,
			(uint32_t)textureWidth,
			(uint32_t)textureHeight,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			_image,
			_imageMemory
		);

		TransitionImageLayout(_device, _image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(_device, stagingBuffer, _image, (uint32_t)textureWidth, (uint32_t)textureHeight);
		TransitionImageLayout(_device, _image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(_device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(_device->logicalDevice, stagingBufferMemory, nullptr);
	}

	// Creates an image, imageView, and sampler
	void CreateTexture(VulkanDevice* _device, const std::string _dir, VkImage& _image, VkDeviceMemory& _imageMemory, VkImageView& _imageView, VkSampler& _imageSampler)
	{
		CreateTextureImage(_device, _dir, _image, _imageMemory);
		CreateTextureImageView(_device, _image, _imageView);
		CreateTextureSampler(_device, _imageSampler);
	}
}

