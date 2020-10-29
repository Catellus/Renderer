#pragma once

#include <vulkan/vulkan.h>

#include "Initializers.h"
#include "VulkanDevice.h"
#include "Mesh.h"
#include "Texture.h"
#include "Lights.h"
#include "Shaders.h"

#include <iostream>

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

// TODO : Only create each image once
// TODO : Share BaseShader between objects using the same shader
class Object
{
// ==============================================
// Variables
// ==============================================
private:
	VulkanDevice* device;
	Mesh mesh;

	skel::MvpInfo mvp;

	// Texture data
	std::vector<VkImage> images;
	std::vector<VkDeviceMemory> imageMemories;

public:
	skel::Transform transform;
	VkDeviceMemory* mvpMemory;
	VkDeviceMemory* lightBufferMemory;
	BaseShader shader;

	std::vector<VkDeviceMemory> bufferMemories;

// ==============================================
// Functions
// ==============================================
public:
	// Builds the object's mesh and loads its textures
	Object(
		VulkanDevice* _device,
		const char* _modelDirectory,
		skel::shaders::ShaderTypes _shaderType
		) : device(_device)
	{
		mvp.model = glm::mat4(1.0f);

		mesh = LoadMesh(device, _modelDirectory);
	}

	// Destroy this object's buffers
	~Object()
	{
		for (auto& memory : bufferMemories)
		{
			vkFreeMemory(device->logicalDevice, memory, nullptr);
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(images.size()); i++)
		{
			vkDestroyImage(device->logicalDevice, images[i], nullptr);
			vkFreeMemory(device->logicalDevice, imageMemories[i], nullptr);
		}

		shader.Cleanup(device->logicalDevice);
		mesh.Cleanup(device->logicalDevice);
	}

	void AttachTexture(const char* _directory)
	{
		VkImage* textureImage = new VkImage();
		images.push_back(*textureImage);
		VkImage& image = images[static_cast<uint32_t>(images.size()) - 1];

		VkDeviceMemory* textureMemory = new VkDeviceMemory();
		imageMemories.push_back(*textureMemory);
		VkDeviceMemory& memory = imageMemories[static_cast<uint32_t>(imageMemories.size()) - 1];

		VkImageView& view = shader.AddImageView();
		VkSampler& sampler = shader.AddSampler();
		skel::CreateTexture(device, _directory, image, memory, view, sampler);
	}

	void AttachBuffer(VkDeviceSize _size)
	{
		VkBuffer& shaderBuffer = shader.AddBuffer();

		VkDeviceMemory* memory = new VkDeviceMemory();
		bufferMemories.push_back(*memory);
		VkDeviceMemory& bufferMemory = bufferMemories[static_cast<uint32_t>(bufferMemories.size()) - 1];

		device->CreateBuffer(
			_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			shaderBuffer,
			bufferMemory
		);
	}

	// Records commands to the input CommandBuffer
	void Draw(VkCommandBuffer _commandBuffer, VkPipelineLayout pipelineLayout)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &mesh.vertexBuffer, offsets);
		vkCmdBindIndexBuffer(_commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &shader.descriptorSet, 0, nullptr);
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
		device->CopyDataToBufferMemory(&mvp, sizeof(mvp), *mvpMemory);
	}

};

