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

// Holds vertex information and its handles descriptions
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescription = {};
		// Position
		attributeDescription[0].binding = 0;
		attributeDescription[0].location = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].offset = offsetof(Vertex, position);
		// Normal
		attributeDescription[1].binding = 0;
		attributeDescription[1].location = 1;
		attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[1].offset = offsetof(Vertex, normal);
		// UV coord
		attributeDescription[2].binding = 0;
		attributeDescription[2].location = 2;
		attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[2].offset = offsetof(Vertex, texCoord);

		return attributeDescription;
	}

	bool operator==(const Vertex& other) const
	{
		return position == other.position && normal == other.normal && texCoord == other.texCoord;
	}
};

// TODO : Make a better hash function http://www.azillionmonkeys.com/qed/hash.html
// Used to map vertices into an unordered array during mesh building
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1) ^
				(hash<glm::vec3>()(vertex.normal) << 1);
		}
	};
}

// Stores basic information to render a model
struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

// A crude candy corn
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

// Loads the object from disk
// Returns a mesh built from the input file
Mesh LoadMesh(const char* _directory)
{
	// Load the mesh with Tinyobj
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _directory))
		throw std::runtime_error(warn + err);

	// Convert the Tinyobj mesh into a Skeleton mesh
	Mesh endMesh = {};
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
				uniqueVerts[vert] = (uint32_t)endMesh.vertices.size();
				endMesh.vertices.push_back(vert);
			}
			endMesh.indices.push_back(uniqueVerts[vert]);
		}
	}

	return endMesh;
}

