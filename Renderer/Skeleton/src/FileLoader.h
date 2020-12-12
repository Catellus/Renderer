#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>

#include "Common.h"
#include "Mesh.h"
#include "Texture.h"

// Loads the object from disk
// Returns a mesh built from the input file
inline Mesh* LoadMesh(VulkanDevice* _device, const char* _directory)
{
	// Load the mesh with Tinyobj
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _directory))
		throw std::runtime_error(warn + err);

	// Convert the Tinyobj mesh into a Skeleton mesh
	Mesh* endMesh = new Mesh();
	std::unordered_map<Vertex, uint32_t> uniqueVerts = {};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vert = {};

			// Tinyobj does not use vec3's -- This makes a vec3 from 3 floats
			vert.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vert.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				// Y axis is 0->1 top->bottom (inverse of OpenGL)
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vert.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			if (uniqueVerts.count(vert) == 0)
			{
				uniqueVerts[vert] = (uint32_t)endMesh->vertices.size();
				endMesh->vertices.push_back(vert);
			}
			endMesh->indices.push_back(uniqueVerts[vert]);
		}
	}

	_device->CreateAndFillBuffer(
		endMesh->vertices.data(),
		sizeof(endMesh->vertices[0]) * static_cast<uint32_t>(endMesh->vertices.size()),
		endMesh->vertexBuffer,
		endMesh->vertexBufferMemory,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);
	_device->CreateAndFillBuffer(
		endMesh->indices.data(),
		sizeof(endMesh->indices[0]) * static_cast<uint32_t>(endMesh->indices.size()),
		endMesh->indexBuffer,
		endMesh->indexBufferMemory,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);

	return endMesh;
}

// Loads the input texture, and copies it to an image
inline void LoadTextureToImage(VulkanDevice* _device, const std::string _directory, VkImage& _image, VkDeviceMemory& _imageMemory)
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

	skel::CreateImage(
		_device,
		(uint32_t)textureWidth,
		(uint32_t)textureHeight,
		//VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		_image,
		_imageMemory
	);

	//TransitionImageLayout(_device, _image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	skel::TransitionImageLayout(_device, _image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	skel::CopyBufferToImage(_device, stagingBuffer, _image, (uint32_t)textureWidth, (uint32_t)textureHeight);
	//TransitionImageLayout(_device, _image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	skel::TransitionImageLayout(_device, _image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(_device->logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(_device->logicalDevice, stagingBufferMemory, nullptr);
}

// Creates an image, imageView, and sampler
inline void CreateTexture(VulkanDevice* _device, const char* _fileName, VkImage& _image, VkDeviceMemory& _imageMemory, VkImageView& _imageView, VkSampler& _imageSampler)
{
	LoadTextureToImage(_device, std::string(texturePrefix) + _fileName, _image, _imageMemory);
	skel::CreateTextureImageView(_device, _image, _imageView);
	skel::CreateTextureSampler(_device, _imageSampler);
}

