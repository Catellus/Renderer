#pragma once

#include <vulkan/vulkan.h>

#include "Initializers.h"
#include "VulkanDevice.h"
#include "Mesh.h"
#include "Texture.h"
#include "Lights.h"
#include "Shaders.h"

namespace skel
{
	// Matrices for translating objects to clip space
	struct MvpInfo {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::vec3 camPosition;
	};

	// Vectors transformed into the object's model matrix
	struct Transform {
		glm::vec3 position	= { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation	= { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale		= { 1.0f, 1.0f, 1.0f };
	};
}

class Object
{
// ==============================================
// Variables
// ==============================================
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
	// Normal
	VkImage normalImage;
	VkDeviceMemory normalImageMemory;

	skel::MvpInfo mvp;
	VkDeviceMemory mvpBufferMemory;

public:
	skel::Transform transform;
	VkDeviceMemory lightBufferMemory;
	skel::shaders::OpaqueInformation defaultShaderInfo;

// ==============================================
// Functions
// ==============================================
public:
	// Builds the object's mesh and loads its textures
	Object(
		VulkanDevice* _device,
		const char* _modelDirectory,
		skel::shaders::ShaderTypes _shaderType,
		const char* _albedoDirectory,
		const char* _normalDirectory
		) : device(_device)
	{
		mvp.model = glm::mat4(1.0f);
		defaultShaderInfo = {};

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

		skel::CreateTexture(device, _albedoDirectory, albedoImage, albedoImageMemory, defaultShaderInfo.albedoImageView, defaultShaderInfo.albedoImageSampler);
		skel::CreateTexture(device, _normalDirectory, normalImage, normalImageMemory, defaultShaderInfo.normalImageView, defaultShaderInfo.normalImageSampler);

		CreateUniformBuffers();
	}

	// Destroy this object's buffers
	~Object()
	{
		vkDestroyBuffer	(device->logicalDevice, vertexBuffer      , nullptr);
		vkFreeMemory	(device->logicalDevice, vertexBufferMemory, nullptr);
		vkDestroyBuffer	(device->logicalDevice, indexBuffer       , nullptr);
		vkFreeMemory	(device->logicalDevice, indexBufferMemory , nullptr);

		vkDestroyBuffer	(device->logicalDevice, defaultShaderInfo.mvpBuffer  , nullptr);
		vkFreeMemory	(device->logicalDevice, mvpBufferMemory       , nullptr);
		vkDestroyBuffer	(device->logicalDevice, defaultShaderInfo.lightBuffer, nullptr);
		vkFreeMemory	(device->logicalDevice, lightBufferMemory     , nullptr);

		vkDestroyImage		(device->logicalDevice, albedoImage                  , nullptr);
		vkFreeMemory		(device->logicalDevice, albedoImageMemory            , nullptr);
		vkDestroyImageView	(device->logicalDevice, defaultShaderInfo.albedoImageView   , nullptr);
		vkDestroySampler	(device->logicalDevice, defaultShaderInfo.albedoImageSampler, nullptr);

		vkDestroyImage		(device->logicalDevice, normalImage                  , nullptr);
		vkFreeMemory		(device->logicalDevice, normalImageMemory            , nullptr);
		vkDestroyImageView	(device->logicalDevice, defaultShaderInfo.normalImageView   , nullptr);
		vkDestroySampler	(device->logicalDevice, defaultShaderInfo.normalImageSampler, nullptr);
	}

	// Create the MVP and light buffers for their uniforms
	void CreateUniformBuffers()
	{
		VkDeviceSize mvpBufferSize = sizeof(skel::MvpInfo);
		VkDeviceSize lightBufferSize = sizeof(skel::lights::ShaderLights);

		device->CreateBuffer(
			mvpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			 defaultShaderInfo.mvpBuffer,
			mvpBufferMemory
		);

		device->CreateBuffer(
			lightBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			defaultShaderInfo.lightBuffer,
			lightBufferMemory
		);
	}

	// Records commands to the input CommandBuffer
	void Draw(VkCommandBuffer _commandBuffer, VkPipelineLayout pipelineLayout)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(_commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &defaultShaderInfo.descriptorSet, 0, nullptr);
		vkCmdDrawIndexed(_commandBuffer, (uint32_t)mesh.indices.size(), 1, 0, 0, 0);
	}

	// Turns the Transform into its model matrix
	// Updates the other matrices
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

