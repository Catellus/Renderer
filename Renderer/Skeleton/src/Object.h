#pragma once

#include "Common.h"

#include "Initializers.h"
#include "VulkanDevice.h"
#include "Mesh.h"
#include "Texture.h"
#include "Lights.h"
#include "Shaders.h"
#include "FileLoader.h"

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

// TODO : Only create each image once
// TODO : Share BaseShader between objects using the same shader
class Object
{
// ==============================================
// Variables
// ==============================================
private:
	VulkanDevice* device;
	Mesh* mesh = nullptr;

	skel::MvpInfo mvp;

	// Texture data
	std::vector<VkImage> images;
	std::vector<VkDeviceMemory> imageMemories;

public:
	skel::Transform transform;
	skel::BaseShader shader;

// ==============================================
// Functions
// ==============================================
public:
	// Builds the object's mesh and loads its textures
	Object(
		VulkanDevice* _device,
		skel::ShaderTypes _shaderType,
		const char* _modelDirectory = nullptr
		) : device(_device)
	{
		shader.type = _shaderType;

		// Guarantee the MVP matrices will be buffer 0
		AttachBuffer(sizeof(skel::MvpInfo));
		mvp.model = glm::mat4(1.0f);

		if (_modelDirectory != nullptr)
			mesh = LoadMesh(device, _modelDirectory);
	}

	// Destroy this object's buffers
	~Object()
	{
		shader.Cleanup(device->logicalDevice);

		if (mesh)
		{
			mesh->Cleanup(device->logicalDevice);
			free(mesh);
		}
	}

	// Binds a texture to the shader & loads the image
	// _directory is from the base texture resource folder
	void AttachTexture(const char* _directory)
	{
		TextureComponent* texture = new TextureComponent();
		shader.textures.push_back(texture);

		CreateTexture(device, _directory, texture->image, texture->memory, texture->view, texture->sampler);
	}

	// Binds a buffer to the shader & allocates memory for it
	// _size is in bytes
	void AttachBuffer(VkDeviceSize _size)
	{
		BufferComponent* component = new BufferComponent();
		shader.buffers.push_back(component);

		device->CreateBuffer(
			_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			component->buffer,
			component->memory
		);
	}

	// Records commands to the input CommandBuffer
	void Draw(VkCommandBuffer _commandBuffer, VkPipelineLayout _pipelineLayout)
	{
		if (!mesh)
			return;

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &mesh->vertexBuffer, offsets);
		vkCmdBindIndexBuffer(_commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &shader.descriptorSet, 0, nullptr);
		vkCmdDrawIndexed(_commandBuffer, (uint32_t)mesh->indices.size(), 1, 0, 0, 0);
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
		device->CopyDataToBufferMemory(&mvp, sizeof(mvp), shader.buffers[0]->memory);
	}

};

} // namespace skel

