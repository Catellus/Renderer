#pragma once

#include <vulkan/vulkan.h>

#include "Initializers.h"
#include "VulkanDevice.h"
#include "Mesh.h"

class Object
{
private:
	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::vec3 camPosition;
	};

	struct LightInformation {
		alignas(16) glm::vec3 color;
		alignas(16) glm::vec3 position;
	};

public:
	VulkanDevice* device;
	Mesh mesh;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkBuffer uboBuffer;
	VkDeviceMemory uboBufferMemory;
	VkBuffer lightBuffer;
	VkDeviceMemory lightBufferMemory;

	// Albedo
	VkImage albedoImage;
	VkDeviceMemory albedoImageMemory;
	VkImageView albedoImageView;
	VkSampler albedoImageSampler;
	// Normal
	VkImage normalImage;
	VkDeviceMemory normalImageMemory;
	VkImageView normalImageView;
	VkSampler normalImageSampler;

	VkDescriptorSet descriptorSet;

public:
	Object(VulkanDevice* _device, const char* _modelDirectory) : device(_device)
	{
		mesh = LoadMesh(_modelDirectory);
		device->CreateVertexBuffer(mesh.vertices, vertexBuffer, vertexBufferMemory);
		device->CreateIndexBuffer(mesh.indices, indexBuffer, indexBufferMemory);
	}

	~Object()
	{
		vkDestroyBuffer	(device->logicalDevice, vertexBuffer      , nullptr);
		vkFreeMemory	(device->logicalDevice, vertexBufferMemory, nullptr);
		vkDestroyBuffer	(device->logicalDevice, indexBuffer       , nullptr);
		vkFreeMemory	(device->logicalDevice, indexBufferMemory , nullptr);

		vkDestroyBuffer	(device->logicalDevice, uboBuffer        , nullptr);
		vkFreeMemory	(device->logicalDevice, uboBufferMemory  , nullptr);
		vkDestroyBuffer	(device->logicalDevice, lightBuffer      , nullptr);
		vkFreeMemory	(device->logicalDevice, lightBufferMemory, nullptr);
	}

	void CreateUniformBuffers()
	{
		VkDeviceSize uboBufferSize = sizeof(UniformBufferObject);
		VkDeviceSize lightBufferSize = sizeof(LightInformation);

		device->CreateBuffer(
			uboBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uboBuffer,
			uboBufferMemory
		);

		device->CreateBuffer(
			lightBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			lightBuffer,
			lightBufferMemory
		);
	}

	void CreateDescriptorSets(VkDescriptorPool _pool, VkDescriptorSetLayout _layout, VkImageView _alV, VkSampler _alS, VkImageView _nV, VkSampler _nS)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_layout;

		if (vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor sets");

		VkDescriptorBufferInfo uboInfo = {};
		uboInfo.offset = 0;
		uboInfo.range = sizeof(UniformBufferObject);
		uboInfo.buffer = uboBuffer;

		VkDescriptorImageInfo albedoInfo = {};
		albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoInfo.imageView = _alV;//albedoImageView;
		albedoInfo.sampler = _alS;//albedoImageSampler;

		VkDescriptorBufferInfo lightInfo = {};
		lightInfo.offset = 0;
		lightInfo.range = sizeof(LightInformation);
		lightInfo.buffer = lightBuffer;

		VkDescriptorImageInfo normalInfo = {};
		normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalInfo.imageView = _nV;//normalImageView;
		normalInfo.sampler = _nS;//normalImageSampler;


		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			skel::initializers::WriteDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				&uboInfo,
				0),
			skel::initializers::WriteDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&albedoInfo,
				1),
			skel::initializers::WriteDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				&lightInfo,
				2),
			skel::initializers::WriteDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&normalInfo,
				3),
		};

		vkUpdateDescriptorSets(device->logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	void Draw(VkCommandBuffer _commandBuffer, VkPipelineLayout pipelineLayout)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(_commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdDrawIndexed(_commandBuffer, (uint32_t)mesh.indices.size(), 1, 0, 0, 0);
	}

};

