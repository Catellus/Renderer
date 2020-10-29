#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Initializers.h"
#include "VulkanDevice.h"
#include "Lights.h"

namespace skel
{
	namespace shaders
	{
		enum ShaderTypes
		{
			Opaque = 0,
			Unlit = 1
		};
	}
}

class BaseShader
{
public:
	skel::shaders::ShaderTypes type;
	VkDescriptorSet descriptorSet;

	std::vector<VkBuffer> buffers = {};
	std::vector<VkImageView> imageViews = {};
	std::vector<VkSampler> samplers = {};

	VkBuffer& AddBuffer()
	{
		VkBuffer* tmpBuffer = new VkBuffer();
		buffers.push_back(*tmpBuffer);
		return buffers[static_cast<uint32_t>(buffers.size()) - 1];
	}

	VkImageView& AddImageView()
	{
		VkImageView* tmpView = new VkImageView();
		imageViews.push_back(*tmpView);
		return imageViews[static_cast<uint32_t>(imageViews.size()) -1];
	}

	VkSampler& AddSampler()
	{
		VkSampler* tmpSampler = new VkSampler();
		samplers.push_back(*tmpSampler);
		return samplers[static_cast<uint32_t>(samplers.size()) - 1];
	}

	void GetDescriptorWriteSets(std::vector<VkWriteDescriptorSet>& _outSet, std::vector<VkDescriptorBufferInfo>& bufferDescriptors, std::vector<VkDescriptorImageInfo>& imageDescriptors)
	{
		uint32_t descriptorIndex = 0;
		uint64_t buffersSize = static_cast<uint64_t>(buffers.size());
		uint64_t imagesSize  = static_cast<uint64_t>(imageViews.size());
		_outSet.resize(buffersSize + imagesSize);
		bufferDescriptors.resize(buffersSize);
		imageDescriptors.resize(imagesSize);

		VkWriteDescriptorSet tmp;

		for (auto& buffer : buffers)
		{
			VkDescriptorBufferInfo bufferDescriptor = {};
			bufferDescriptor.offset = 0;
			bufferDescriptor.range = VK_WHOLE_SIZE;
			bufferDescriptor.buffer = buffer;
			bufferDescriptors[descriptorIndex] = bufferDescriptor;
			tmp = skel::initializers::WriteDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferDescriptors[descriptorIndex], descriptorIndex);
			_outSet[descriptorIndex] = tmp;

			descriptorIndex++;
		}

		for (uint32_t i = 0; i < imagesSize; i++)
		{
			VkDescriptorImageInfo imageDescriptor = {};
			imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageDescriptor.imageView = imageViews[i];
			imageDescriptor.sampler = samplers[i];
			imageDescriptors[i] = imageDescriptor;
			tmp = skel::initializers::WriteDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageDescriptors[i], descriptorIndex);
			_outSet[descriptorIndex] = tmp;

			descriptorIndex++;
		}

	}

	void Cleanup(VkDevice& _device)
	{
		for (auto buffer : buffers)
		{
			vkDestroyBuffer(_device, buffer, nullptr);
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(imageViews.size()); i++)
		{
			vkDestroyImageView(_device, imageViews[i], nullptr);
			vkDestroySampler(_device, samplers[i], nullptr);
		}
	}
};

namespace skel
{
	namespace shaders
	{
		// Handles the descriptor information for the render pipeline about opaque shaders
		struct ShaderDescriptorInformation
		{
			VkDescriptorSetLayout descriptorSetLayout;
			VkDescriptorPool descriptorPool;

			void CreateDescriptorSetLayout(VkDevice& _device)
			{
				std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
					skel::initializers::DescriptorSetLyoutBinding(	// MVP matrices
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						VK_SHADER_STAGE_VERTEX_BIT,
						0
					),
					skel::initializers::DescriptorSetLyoutBinding(	// Light infos
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						1
					),
					skel::initializers::DescriptorSetLyoutBinding(	// Albedo map
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						2
					),
					skel::initializers::DescriptorSetLyoutBinding(	// Normal map
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						3
					)
				};

				VkDescriptorSetLayoutCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				createInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
				createInfo.pBindings = layoutBindings.data();

				if (vkCreateDescriptorSetLayout(_device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
					throw std::runtime_error("Failed to create a descriptor set layout");
			}

			void CreateDescriptorPool(VkDevice& _device, uint32_t _objectCount)
			{
				std::vector<VkDescriptorPoolSize> poolSizes = {
					skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _objectCount),			// MVP matrices
					skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _objectCount),			// Light
					skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _objectCount),	// Albeto Map
					skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _objectCount)		// Normal Map
				};

				VkDescriptorPoolCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
				createInfo.pPoolSizes = poolSizes.data();
				createInfo.maxSets = _objectCount;

				if (vkCreateDescriptorPool(_device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
					throw std::runtime_error("Failed to create descriptor pool");
			}

			void CreateDescriptorSets(VkDevice& _device, BaseShader& _shaderInfo)
			{
				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = descriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &descriptorSetLayout;

				if (vkAllocateDescriptorSets(_device, &allocInfo, &_shaderInfo.descriptorSet) != VK_SUCCESS)
					throw std::runtime_error("Failed to create descriptor sets");

				std::vector<VkWriteDescriptorSet> descriptorWrites;
				std::vector<VkDescriptorBufferInfo> bufferInfos;
				std::vector<VkDescriptorImageInfo> imageInfos;
				_shaderInfo.GetDescriptorWriteSets(descriptorWrites, bufferInfos, imageInfos);

				vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}

			void Cleanup(VkDevice& _device)
			{
				vkDestroyDescriptorSetLayout(_device, descriptorSetLayout, nullptr);
				vkDestroyDescriptorPool(_device, descriptorPool, nullptr);
			}
		};

	}
}

