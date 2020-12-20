#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

enum ComponentTypes
{
	SKEL_COMPONENT_BUFFER = 0,
	SKEL_COMPONENT_SHADER = 1
};

struct ECSComponent
{
	ComponentTypes type;
};

template<typename T>
struct BaseComponent : public ECSComponent
{
	
};

struct BufferComponent : public BaseComponent<BufferComponent>
{
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct TextureComponent : public BaseComponent<TextureComponent>
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
};

