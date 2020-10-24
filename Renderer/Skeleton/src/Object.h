#pragma once

#include <vulkan/vulkan.h>

#include "Initializers.h"
#include "VulkanDevice.h"
#include "Mesh.h"
#include "Texture.h"
#include "Lights.h"

namespace skel
{
	struct MvpInfo {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::vec3 camPosition;
	};

	struct Transform {
		glm::vec3 position	= { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation	= { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale		= { 1.0f, 1.0f, 1.0f };
	};
}

class Object
{
private:
	VulkanDevice* device;
	Mesh mesh;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

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

	skel::MvpInfo mvp;
	VkBuffer mvpBuffer;
	VkDeviceMemory mvpBufferMemory;

public:
	skel::Transform transform;
	VkBuffer lightBuffer;
	VkDeviceMemory lightBufferMemory;

public:
	Object(VulkanDevice* _device, const char* _modelDirectory, const char* _albedoDirectory, const char* _normalDirectory) : device(_device)
	{
		mvp.model = glm::mat4(1.0f);

		mesh = LoadMesh(_modelDirectory);
		device->CreateAndFillBuffer(
			mesh.vertices.data(),
			sizeof(mesh.vertices[0]) * static_cast<uint32_t>(mesh.vertices.size()),
			vertexBuffer,
			vertexBufferMemory,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);
		device->CreateAndFillBuffer(
			mesh.indices.data(),
			sizeof(mesh.indices[0]) * static_cast<uint32_t>(mesh.indices.size()),
			indexBuffer,
			indexBufferMemory,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);

		if (_albedoDirectory)
		{
			skel::CreateTexture(device, _albedoDirectory, albedoImage, albedoImageMemory, albedoImageView, albedoImageSampler);
		}
		if (_normalDirectory)
		{
			skel::CreateTexture(device, _normalDirectory, normalImage, normalImageMemory, normalImageView, normalImageSampler);
		}

		CreateUniformBuffers();
	}

	~Object()
	{
		vkDestroyBuffer	(device->logicalDevice, vertexBuffer      , nullptr);
		vkFreeMemory	(device->logicalDevice, vertexBufferMemory, nullptr);
		vkDestroyBuffer	(device->logicalDevice, indexBuffer       , nullptr);
		vkFreeMemory	(device->logicalDevice, indexBufferMemory , nullptr);

		vkDestroyBuffer	(device->logicalDevice, mvpBuffer        , nullptr);
		vkFreeMemory	(device->logicalDevice, mvpBufferMemory  , nullptr);
		vkDestroyBuffer	(device->logicalDevice, lightBuffer      , nullptr);
		vkFreeMemory	(device->logicalDevice, lightBufferMemory, nullptr);

		vkDestroyImage		(device->logicalDevice, albedoImage       , nullptr);
		vkFreeMemory		(device->logicalDevice, albedoImageMemory , nullptr);
		vkDestroyImageView	(device->logicalDevice, albedoImageView   , nullptr);
		vkDestroySampler	(device->logicalDevice, albedoImageSampler, nullptr);

		vkDestroyImage		(device->logicalDevice, normalImage       , nullptr);
		vkFreeMemory		(device->logicalDevice, normalImageMemory , nullptr);
		vkDestroyImageView	(device->logicalDevice, normalImageView   , nullptr);
		vkDestroySampler	(device->logicalDevice, normalImageSampler, nullptr);
	}

	void CreateUniformBuffers()
	{
		VkDeviceSize mvpBufferSize = sizeof(skel::MvpInfo);
		VkDeviceSize lightBufferSize = sizeof(skel::lights::SpotLight);

		device->CreateBuffer(
			mvpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			 mvpBuffer,
			mvpBufferMemory
		);

		device->CreateBuffer(
			lightBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			lightBuffer,
			lightBufferMemory
		);
	}

	void CreateDescriptorSets(VkDescriptorPool _pool, VkDescriptorSetLayout _layout)
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
		uboInfo.range = sizeof(skel::MvpInfo);
		uboInfo.buffer = mvpBuffer;

		VkDescriptorImageInfo albedoInfo = {};
		albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoInfo.imageView = albedoImageView;
		albedoInfo.sampler = albedoImageSampler;

		VkDescriptorBufferInfo lightInfo = {};
		lightInfo.offset = 0;
		lightInfo.range = sizeof(skel::lights::SpotLight);
		lightInfo.buffer = lightBuffer;

		VkDescriptorImageInfo normalInfo = {};
		normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalInfo.imageView = normalImageView;
		normalInfo.sampler = normalImageSampler;


		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			skel::initializers::WriteDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				&uboInfo,
				0),
			//skel::initializers::WriteDescriptorSet(
			//	descriptorSet,
			//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			//	&albedoInfo,
			//	1),
			skel::initializers::WriteDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				&lightInfo,
				1),
			//skel::initializers::WriteDescriptorSet(
			//	descriptorSet,
			//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			//	&normalInfo,
			//	3),
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

	void UpdateMVPBuffer(glm::vec3 _camPosition, glm::mat4 _projection, glm::mat4 _view)
	{
		mvp.model = glm::translate(glm::mat4(1.0f), transform.position);
		glm::fquat rotationQuaternion = {glm::radians(transform.rotation)};
		mvp.model *= glm::mat4_cast(rotationQuaternion);
		mvp.model = glm::scale(mvp.model, transform.scale);

		mvp.view		= _view;
		mvp.proj		= _projection;
		mvp.camPosition = _camPosition;
		device->CopyDataToBufferMemory(&mvp, sizeof(mvp), mvpBufferMemory);
	}

};

