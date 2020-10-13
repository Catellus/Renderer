#pragma once

#include <vulkan/vulkan.h>

namespace skel
{
	namespace initializers
	{
		inline VkDescriptorPoolSize DescriptorPoolSize(
			VkDescriptorType _type,
			uint32_t _count
			)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.descriptorCount = _count;
			poolSize.type = _type;
			return poolSize;
		}

		inline VkDescriptorSetLayoutBinding DescriptorSetLyoutBinding(
			VkDescriptorType _type,
			VkShaderStageFlags _stage,
			uint32_t _binding,
			uint32_t _count = 1
			)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.descriptorType = _type;
			layoutBinding.stageFlags = _stage;
			layoutBinding.binding = _binding;
			layoutBinding.descriptorCount = _count;
			layoutBinding.pImmutableSamplers = nullptr;
			return layoutBinding;
		}

		inline VkWriteDescriptorSet WriteDescriptorSet(
			VkDescriptorSet _dstSet,
			VkDescriptorType _type,
			VkDescriptorBufferInfo* _bufferInfo,
			uint32_t _binding,
			uint32_t _count = 1
			)
		{
			VkWriteDescriptorSet writeDescriptorSet {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = _dstSet;
			writeDescriptorSet.descriptorType = _type;
			writeDescriptorSet.pBufferInfo = _bufferInfo;
			writeDescriptorSet.dstBinding = _binding;
			writeDescriptorSet.descriptorCount = _count;
			writeDescriptorSet.dstArrayElement = 0;
			return writeDescriptorSet;
		}

		inline VkWriteDescriptorSet WriteDescriptorSet(
			VkDescriptorSet _dstSet,
			VkDescriptorType _type,
			VkDescriptorImageInfo* _imageInfo,
			uint32_t _binding,
			uint32_t _count = 1
		)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = _dstSet;
			writeDescriptorSet.descriptorType = _type;
			writeDescriptorSet.pImageInfo = _imageInfo;
			writeDescriptorSet.dstBinding = _binding;
			writeDescriptorSet.descriptorCount = _count;
			writeDescriptorSet.dstArrayElement = 0;
			return writeDescriptorSet;
		}

	}
}

