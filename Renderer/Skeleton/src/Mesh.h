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

struct Vertex {
	glm::vec3 position;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec4 tangent;
	//glm::vec3 bitangent;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescription = {};
		// Position
		attributeDescription[0].binding = 0;
		attributeDescription[0].location = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].offset = offsetof(Vertex, position);
		// UV coord
		attributeDescription[1].binding = 0;
		attributeDescription[1].location = 1;
		attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[1].offset = offsetof(Vertex, texCoord);
		// Normal
		attributeDescription[2].binding = 0;
		attributeDescription[2].location = 2;
		attributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[2].offset = offsetof(Vertex, normal);
		// Tangent
		attributeDescription[3].binding = 0;
		attributeDescription[3].location = 3;
		attributeDescription[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescription[3].offset = offsetof(Vertex, tangent);
		//// Bitangent
		//attributeDescription[4].binding = 0;
		//attributeDescription[4].location = 4;
		//attributeDescription[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		//attributeDescription[4].offset = offsetof(Vertex, bitangent);

		return attributeDescription;
	}

	bool operator==(const Vertex& other) const
	{
		return position == other.position && texCoord == other.texCoord && normal == other.normal && tangent == other.tangent;
	}
};

// http://www.azillionmonkeys.com/qed/hash.html
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1) ^
				(hash<glm::vec3>()(vertex.normal) << 1) ^
				(hash<glm::vec3>()(vertex.tangent) << 1);
				//(hash<glm::vec3>()(vertex.bitangent) << 1);
		}
	};
}

struct TemporaryVertexTangentData
{
	glm::vec3 tan;
	glm::vec3 bitan;
};

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

			if (uniqueVerts.count(vert) == 0)
			{
				uniqueVerts[vert] = (uint32_t)tmpMesh.vertices.size();
				tmpMesh.vertices.push_back(vert);
			}
			tmpMesh.indices.push_back(uniqueVerts[vert]);
		}
	}

// Calculate Tangent & Bitangent --> https://web.archive.org/web/20140203235513/http://www.terathon.com/code/tangent.html
// TBN in shaders--> https://www.gamasutra.com/blogs/RobertBasler/20131122/205462/Three_Normal_Mapping_Techniques_Explained_For_the_Mathematically_Uninclined.php?print=1
// TODO(Yeager) : average the Tangents & Bitangents from every triangle the vert is a part of
	//glm::vec3 Q1, Q2;
	//glm::vec3 T, B;
	//glm::vec2 st1, st2;
	float r;
	//glm::vec3 tan, bit;

	std::vector<TemporaryVertexTangentData> tanData(static_cast<uint32_t>(tmpMesh.vertices.size()));

	for (uint32_t i = 0; i < (uint32_t)tmpMesh.indices.size(); i += 3)
	{
		uint32_t idx0 = tmpMesh.indices[i    ];
		uint32_t idx1 = tmpMesh.indices[i + 1];
		uint32_t idx2 = tmpMesh.indices[i + 2];

		Vertex v1 = tmpMesh.vertices[idx0];
		Vertex v2 = tmpMesh.vertices[idx1];
		Vertex v3 = tmpMesh.vertices[idx2];

		glm::vec3 p1 = v2.position - v1.position;
		glm::vec3 p2 = v3.position - v1.position;

		glm::vec2 u1 = v2.texCoord - v1.texCoord;
		glm::vec2 u2 = v3.texCoord - v1.texCoord;

		r = 1.0f / (u1.x * u2.y - u1.y * u2.x);
		glm::vec3 tan = r * (u2.y * p1 - u1.y * p2);
		glm::vec3 bitan = r * (u1.x * p2 - u2.x * p1);

		tanData[idx0].tan += tan;
		tanData[idx1].tan += tan;
		tanData[idx2].tan += tan;

		tanData[idx0].bitan += bitan;
		tanData[idx1].bitan += bitan;
		tanData[idx2].bitan += bitan;
	}

	for (uint32_t i = 0; i < static_cast<uint32_t>(tmpMesh.vertices.size()); i++)
	{
		const glm::vec3& n = tmpMesh.vertices[i].normal;
		const glm::vec3& t = tanData[i].tan;

		// Gram-schmidt orthogonalize
		glm::vec3 ot = glm::normalize((t - n * glm::dot(n, t)));

		// handedness
		float w = (glm::dot(glm::cross(n, t), tanData[i].bitan) < 0.0f) ? -1.0f : 1.0f;

		tmpMesh.vertices[i].tangent = glm::vec4(ot.x, ot.y, ot.z, w);
	}

	std::printf("%s\n\tVerts: %d\n\tIndices: %d\n", _directory, (uint32_t)tmpMesh.vertices.size(), (uint32_t)tmpMesh.indices.size());

	tanData.clear();

	return tmpMesh;
}

