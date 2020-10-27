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

// ==============================================
// OPAQUE
// ==============================================

		// Information used by opaque shaders
		struct OpaqueInformation
		{
			VkBuffer mvpBuffer;		// skel::mvpInfo
			VkBuffer lightBuffer;	// skel::lights::ShaderLights

			VkImageView albedoImageView;
			VkSampler albedoImageSampler;
			VkImageView normalImageView;
			VkSampler normalImageSampler;

			VkDescriptorSet descriptorSet;
		};

 // TODO : Generalize this
		// Handles the descriptor information for the render pipeline about opaque shaders
		struct OpaqueDescriptorInformation
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

			void CreateDescriptorSets(VkDevice& _device, skel::shaders::OpaqueInformation& _shaderInfo)
			{
				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = descriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &descriptorSetLayout;

				if (vkAllocateDescriptorSets(_device, &allocInfo, &_shaderInfo.descriptorSet) != VK_SUCCESS)
					throw std::runtime_error("Failed to create descriptor sets");

				VkDescriptorBufferInfo mvpInfo = {};
				mvpInfo.offset = 0;
				mvpInfo.range = VK_WHOLE_SIZE; //sizeof(skel::MvpInfo);
				mvpInfo.buffer = _shaderInfo.mvpBuffer;

				VkDescriptorBufferInfo lightInfo = {};
				lightInfo.offset = 0;
				lightInfo.range = VK_WHOLE_SIZE; //sizeof(skel::lights::ShaderLights);
				lightInfo.buffer = _shaderInfo.lightBuffer;

				VkDescriptorImageInfo albedoInfo = {};
				albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				albedoInfo.imageView = _shaderInfo.albedoImageView;
				albedoInfo.sampler = _shaderInfo.albedoImageSampler;

				VkDescriptorImageInfo normalInfo = {};
				normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				normalInfo.imageView = _shaderInfo.normalImageView;
				normalInfo.sampler = _shaderInfo.normalImageSampler;

				std::vector<VkWriteDescriptorSet> descriptorWrites = {
					skel::initializers::WriteDescriptorSet(	// MVP matrices
						_shaderInfo.descriptorSet,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						&mvpInfo,
						0),
					skel::initializers::WriteDescriptorSet(	// Light infos
						_shaderInfo.descriptorSet,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						&lightInfo,
						1),
					skel::initializers::WriteDescriptorSet(	// Albedo map
						_shaderInfo.descriptorSet,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						&albedoInfo,
						2),
					skel::initializers::WriteDescriptorSet(	// Normal map
						_shaderInfo.descriptorSet,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						&normalInfo,
						3),
				};

				vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}

			void Cleanup(VkDevice& _device)
			{
				vkDestroyDescriptorSetLayout(_device, descriptorSetLayout, nullptr);
				vkDestroyDescriptorPool(_device, descriptorPool, nullptr);
			}
		};

// ==============================================
// UNLIT
// ==============================================

		// Information used by unlit shaders
		struct UnlitInformation
		{
			VkBuffer mvpBuffer;		// skel::mvpInfo
			VkBuffer objectColor;

			VkDescriptorSet descriptorSet;
		};

		// Handles the descriptor information for the render pipeline about unlit shaders
		struct UnlitDescriptorInformation
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
					skel::initializers::DescriptorSetLyoutBinding(	// Object color
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						1
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
					skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _objectCount),	// MVP matrices
					skel::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _objectCount),	// Object color
				};

				VkDescriptorPoolCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
				createInfo.pPoolSizes = poolSizes.data();
				createInfo.maxSets = _objectCount;

				if (vkCreateDescriptorPool(_device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
					throw std::runtime_error("Failed to create descriptor pool");
			}

			void CreateDescriptorSets(VkDevice& _device, skel::shaders::UnlitInformation& _shaderInfo)
			{
				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = descriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &descriptorSetLayout;

				if (vkAllocateDescriptorSets(_device, &allocInfo, &_shaderInfo.descriptorSet) != VK_SUCCESS)
					throw std::runtime_error("Failed to create descriptor sets");

				VkDescriptorBufferInfo mvpInfo = {};
				mvpInfo.offset = 0;
				mvpInfo.range = VK_WHOLE_SIZE; //sizeof(skel::MvpInfo);
				mvpInfo.buffer = _shaderInfo.mvpBuffer;

				VkDescriptorBufferInfo objectColorInfo = {};
				objectColorInfo.offset = 0;
				objectColorInfo.range = VK_WHOLE_SIZE; //sizeof(glm::vec3);
				objectColorInfo.buffer = _shaderInfo.objectColor;

				std::vector<VkWriteDescriptorSet> descriptorWrites = {
					skel::initializers::WriteDescriptorSet(	// MVP matrices
						_shaderInfo.descriptorSet,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						&mvpInfo,
						0),
					skel::initializers::WriteDescriptorSet(	// Object color
						_shaderInfo.descriptorSet,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						&objectColorInfo,
						1)
				};

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

