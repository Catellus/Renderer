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

#include "Mesh.h"

Mesh LoadModel(const std::string _directory)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _directory.c_str()))
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
	for (uint32_t i = 0; i < (uint32_t)tmpMesh.indices.size(); i+=3)
	{
		Vertex& vertOne = tmpMesh.vertices[tmpMesh.indices[i  ]];
		Vertex& vertTwo = tmpMesh.vertices[tmpMesh.indices[i+1]];
		Vertex& vertThr = tmpMesh.vertices[tmpMesh.indices[i+2]];

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

	return tmpMesh;
}

