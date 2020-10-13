#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED // Sort of breaks GLSL (ALl Zs need to be flipped)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

//// Mind the alignment
//struct UniformBufferObject {
//	alignas(16) glm::mat4 model;
//	alignas(16) glm::mat4 view;
//	alignas(16) glm::mat4 proj;
//	alignas(16) glm::vec3 camPosition;
//};

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	static std::array<VkVertexInputAttributeDescription, 6> GetAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 6> attributeDescription = {};
		// Position
		attributeDescription[0].binding = 0;
		attributeDescription[0].location = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].offset = offsetof(Vertex, position);
		// Color
		attributeDescription[1].binding = 0;
		attributeDescription[1].location = 1;
		attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[1].offset = offsetof(Vertex, color);
		// UV coord
		attributeDescription[2].binding = 0;
		attributeDescription[2].location = 2;
		attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[2].offset = offsetof(Vertex, texCoord);
		// Normal
		attributeDescription[3].binding = 0;
		attributeDescription[3].location = 3;
		attributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[3].offset = offsetof(Vertex, normal);
		// Tangent
		attributeDescription[4].binding = 0;
		attributeDescription[4].location = 4;
		attributeDescription[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[4].offset = offsetof(Vertex, tangent);
		// Bitangent
		attributeDescription[5].binding = 0;
		attributeDescription[5].location = 5;
		attributeDescription[5].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[5].offset = offsetof(Vertex, bitangent);

		return attributeDescription;
	}

	bool operator==(const Vertex& other) const
	{
		return position == other.position && color == other.color && texCoord == other.texCoord && normal == other.normal;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1) ^
				(hash<glm::vec3>()(vertex.normal) << 1) ^
				(hash<glm::vec3>()(vertex.tangent) << 1) ^
				(hash<glm::vec3>()(vertex.bitangent) << 1);
		}
	};
}

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

//const std::vector<Vertex> verts = {
//	{{ 0.0f  , -0.65f, 0.0f}, {1.0f, 1.0f , 1.0f }, {0.0f, 0.0f}}, //0
//	{{ 0.1f  , -0.6f , 0.0f}, {1.0f, 1.0f , 1.0f }, {0.0f, 0.0f}}, //1
//	{{-0.1f  , -0.6f , 0.0f}, {1.0f, 1.0f , 1.0f }, {0.0f, 0.0f}}, //2
//	{{ 0.25f , -0.2f , 0.0f}, {1.0f, 0.32f, 0.02f}, {0.0f, 0.0f}}, //3
//	{{-0.25f , -0.2f , 0.0f}, {1.0f, 0.32f, 0.02f}, {0.0f, 0.0f}}, //4
//	{{ 0.35f ,  0.2f , 0.0f}, {1.0f, 0.32f, 0.02f}, {0.0f, 0.0f}}, //5
//	{{-0.35f ,  0.2f , 0.0f}, {1.0f, 0.32f, 0.02f}, {0.0f, 0.0f}}, //6
//	{{ 0.4f  ,  0.5f , 0.0f}, {1.0f, 0.86f, 0.0f }, {0.0f, 0.0f}}, //7
//	{{-0.4f  ,  0.5f , 0.0f}, {1.0f, 0.86f, 0.0f }, {0.0f, 0.0f}}, //8
//	{{ 0.325f,  0.6f , 0.0f}, {1.0f, 0.86f, 0.0f }, {0.0f, 0.0f}}, //9
//	{{-0.325f,  0.6f , 0.0f}, {1.0f, 0.86f, 0.0f }, {0.0f, 0.0f}}, //10
//	{{ 0.2f  ,  0.65f, 0.0f}, {1.0f, 0.86f, 0.0f }, {0.0f, 0.0f}}, //11
//	{{-0.2f  ,  0.65f, 0.0f}, {1.0f, 0.86f, 0.0f }, {0.0f, 0.0f}}, //12
//	{{ 0.0f  ,  0.7f , 0.0f}, {1.0f, 0.86f, 0.0f }, {0.0f, 0.0f}}, //13
//};

//const std::vector<uint16_t> indices = {
//	0, 1, 2,
//	2, 1, 3, 3, 4, 2,
//	4, 3, 5, 5, 6, 4,
//	5, 7, 6, 6, 7, 8,
//	7, 9, 8, 8, 9, 10,
//	9, 11, 10, 10, 11, 12,
//	11, 13, 12,
//};

Mesh LoadMesh(const char* _directory)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _directory))
		throw std::runtime_error(warn + err);

	Mesh tmpMesh = {};
	std::unordered_map<Vertex, uint32_t> uniqueVerts = {};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vert = {};

			vert.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vert.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vert.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vert.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVerts.count(vert) == 0)
			{
				uniqueVerts[vert] = (uint32_t)tmpMesh.vertices.size();
				tmpMesh.vertices.push_back(vert);
			}
			tmpMesh.indices.push_back(uniqueVerts[vert]);
		}
	}

	glm::vec3 tangent, bitangent;
	glm::vec3 edge1, edge2;
	glm::vec2 deltaUV1, deltaUV2;
	float f;
	for (uint32_t i = 0; i < (uint32_t)tmpMesh.indices.size(); i += 3)
	{
		Vertex& vertOne = tmpMesh.vertices[tmpMesh.indices[i]];
		Vertex& vertTwo = tmpMesh.vertices[tmpMesh.indices[i + 1]];
		Vertex& vertThr = tmpMesh.vertices[tmpMesh.indices[i + 2]];

		edge1 = vertTwo.position - vertOne.position;
		edge2 = vertThr.position - vertOne.position;
		deltaUV1 = vertTwo.texCoord - vertOne.texCoord;
		deltaUV2 = vertThr.texCoord - vertOne.texCoord;

		f = 1.0f / (deltaUV1.r * deltaUV2.g - deltaUV2.r * deltaUV1.g);

		tangent.r = f * (deltaUV2.g * edge1.r - deltaUV1.g * edge2.r);
		tangent.g = f * (deltaUV2.g * edge1.g - deltaUV1.g * edge2.g);
		tangent.b = f * (deltaUV2.g * edge1.b - deltaUV1.g * edge2.b);

		bitangent.r = f * (-deltaUV2.r * edge1.r + deltaUV1.r * edge2.r);
		bitangent.g = f * (-deltaUV2.r * edge1.g + deltaUV1.r * edge2.g);
		bitangent.b = f * (-deltaUV2.r * edge1.b + deltaUV1.r * edge2.b);

		vertOne.tangent = tangent;
		vertOne.bitangent = bitangent;
		vertTwo.tangent = tangent;
		vertTwo.bitangent = bitangent;
		vertThr.tangent = tangent;
		vertThr.bitangent = bitangent;
	}

	std::printf("%s\n\tVerts: %d\n\tIndices: %d\n", _directory, (uint32_t)tmpMesh.vertices.size(), (uint32_t)tmpMesh.indices.size());
	return tmpMesh;
}

