#pragma once

//#define GLM_FORCE_LEFT_HANDED // Sort of breaks GLSL (ALl Zs need to be flipped)
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

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

