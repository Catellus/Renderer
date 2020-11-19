#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Initializers.h"
#include "VulkanDevice.h"
#include "Lights.h"

#include "ecs/ecs.h"

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

	std::vector<BufferComponent*> buffers = {};
	std::vector<TextureComponent*> textures = {};

	void GetDescriptorWriteSets(std::vector<VkWriteDescriptorSet>& _outSet, std::vector<VkDescriptorBufferInfo>& bufferDescriptors, std::vector<VkDescriptorImageInfo>& imageDescriptors)
	{
		uint32_t descriptorIndex = 0;
		uint64_t buffersSize = static_cast<uint64_t>(buffers.size());
		uint64_t imagesSize  = static_cast<uint64_t>(textures.size());
		_outSet.resize(buffersSize + imagesSize);
		bufferDescriptors.resize(buffersSize);
		imageDescriptors.resize(imagesSize);

		VkWriteDescriptorSet tmp;

		for (auto& buffer : buffers)
		{
			VkDescriptorBufferInfo bufferDescriptor = {};
			bufferDescriptor.offset = 0;
			bufferDescriptor.range = VK_WHOLE_SIZE;
			bufferDescriptor.buffer = buffer->buffer;
			bufferDescriptors[descriptorIndex] = bufferDescriptor;
			tmp = skel::initializers::WriteDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferDescriptors[descriptorIndex], descriptorIndex);
			_outSet[descriptorIndex] = tmp;

			descriptorIndex++;
		}

		for (uint32_t i = 0; i < imagesSize; i++)
		{
			VkDescriptorImageInfo imageDescriptor = {};
			imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageDescriptor.imageView = textures[i]->view;
			imageDescriptor.sampler = textures[i]->sampler;
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
			vkDestroyBuffer(_device, buffer->buffer, nullptr);
			vkFreeMemory(_device, buffer->memory, nullptr);
		}

		for (auto& tex : textures)
		{
			vkDestroyImage(_device, tex->image, nullptr);
			vkDestroyImageView(_device, tex->view, nullptr);
			vkDestroySampler(_device, tex->sampler, nullptr);
			vkFreeMemory(_device, tex->memory, nullptr);
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

			void CreateLayoutBindingsAndPool(VkDevice& _device, const std::vector<VkDescriptorSetLayoutBinding>& _bindings, uint32_t _objectCount)
			{
				// Create Descriptor Set Layout
				// ==============================

				VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCreateInfo.bindingCount = static_cast<uint32_t>(_bindings.size());
				layoutCreateInfo.pBindings = _bindings.data();

				if (vkCreateDescriptorSetLayout(_device, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
					throw std::runtime_error("Failed to create a descriptor set layout");

				// Create Descriptor Pool
				// ==============================
				uint32_t bindingsCount = static_cast<uint32_t>(_bindings.size());

				std::vector<VkDescriptorPoolSize> poolSizes(bindingsCount);
				for (uint32_t i = 0; i < bindingsCount; i++)
				{
					poolSizes[i] = skel::initializers::DescriptorPoolSize(_bindings[i].descriptorType, _objectCount);
				}

				VkDescriptorPoolCreateInfo poolCreateInfo = {};
				poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
				poolCreateInfo.pPoolSizes = poolSizes.data();
				poolCreateInfo.maxSets = _objectCount;

				if (vkCreateDescriptorPool(_device, &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
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

