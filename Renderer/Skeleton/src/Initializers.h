#pragma once

#include <vulkan/vulkan.h>

namespace skel
{
	namespace initializers
	{
		inline VkDescriptorPoolSize DescriptorPoolSize(
			VkDescriptorType _type,
			uint32_t _count
			)
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.descriptorCount = _count;
			poolSize.type = _type;
			return poolSize;
		}

		inline VkDescriptorSetLayoutBinding DescriptorSetLyoutBinding(
			VkDescriptorType _type,
			VkShaderStageFlags _stage,
			uint32_t _binding,
			uint32_t _count = 1
			)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.descriptorType = _type;
			layoutBinding.stageFlags = _stage;
			layoutBinding.binding = _binding;
			layoutBinding.descriptorCount = _count;
			layoutBinding.pImmutableSamplers = nullptr;
			return layoutBinding;
		}

		inline VkWriteDescriptorSet WriteDescriptorSet(
			VkDescriptorSet _dstSet,
			VkDescriptorType _type,
			VkDescriptorBufferInfo* _bufferInfo,
			uint32_t _binding,
			uint32_t _count = 1
			)
		{
			VkWriteDescriptorSet writeDescriptorSet {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = _dstSet;
			writeDescriptorSet.descriptorType = _type;
			writeDescriptorSet.pBufferInfo = _bufferInfo;
			writeDescriptorSet.dstBinding = _binding;
			writeDescriptorSet.descriptorCount = _count;
			writeDescriptorSet.dstArrayElement = 0;
			return writeDescriptorSet;
		}

		inline VkWriteDescriptorSet WriteDescriptorSet(
			VkDescriptorSet _dstSet,
			VkDescriptorType _type,
			VkDescriptorImageInfo* _imageInfo,
			uint32_t _binding,
			uint32_t _count = 1
		)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = _dstSet;
			writeDescriptorSet.descriptorType = _type;
			writeDescriptorSet.pImageInfo = _imageInfo;
			writeDescriptorSet.dstBinding = _binding;
			writeDescriptorSet.descriptorCount = _count;
			writeDescriptorSet.dstArrayElement = 0;
			return writeDescriptorSet;
		}

		inline VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(
			const VkDescriptorSetLayout* _layouts,
			uint32_t _layoutCount = 1
			)
		{
			VkPipelineLayoutCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			createInfo.pSetLayouts = _layouts;
			createInfo.setLayoutCount = _layoutCount;
			return createInfo;
		}

		inline VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo(
			VkPipelineLayout _layout,
			VkRenderPass _renderpass,
			VkPipelineCreateFlags _flags = 0
			)
		{
			VkGraphicsPipelineCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			createInfo.layout = _layout;
			createInfo.renderPass = _renderpass;
			createInfo.flags = _flags;
			return createInfo;
		}

		inline VkComputePipelineCreateInfo ComputePipelineCreateInfo(
			VkPipelineLayout _layout,
			VkPipelineCreateFlags _flags = 0
		)
		{
			VkComputePipelineCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			createInfo.layout = _layout;
			createInfo.flags = _flags;
			return createInfo;
		}

		//inline VkVertexInputBindingDescription VertexInputBindingDescription()
		//{
		//	VkVertexInputBindingDescription createInfo = {};
		//	createInfo.binding = _binding;
		//	createInfo.stride = _stride,
		//	createInfo.inputRate = _inputRate;
		//	return createInfo;
		//}

		inline VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo(
			uint32_t _bindingCount,
			VkVertexInputBindingDescription* _bindings,
			uint32_t _attributeCount,
			VkVertexInputAttributeDescription* _attributes
			)
		{
			VkPipelineVertexInputStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			createInfo.vertexBindingDescriptionCount = _bindingCount;
			createInfo.pVertexBindingDescriptions = _bindings;
			createInfo.vertexAttributeDescriptionCount = _attributeCount;
			createInfo.pVertexAttributeDescriptions = _attributes;
			return createInfo;
		}

		inline VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(
			VkPrimitiveTopology _topology,
			VkBool32 _primitiveRestartEnable = VK_FALSE,
			VkPipelineInputAssemblyStateCreateFlags _flags = 0
			)
		{
			VkPipelineInputAssemblyStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			createInfo.topology = _topology;
			createInfo.flags = _flags;
			createInfo.primitiveRestartEnable = _primitiveRestartEnable;
			return createInfo;
		}

		inline VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(
			uint32_t _viewportCount,
			const VkViewport* _viewports,
			uint32_t _scissorCount,
			const VkRect2D* _scissors,
			VkPipelineViewportStateCreateFlags _flags = 0
			)
		{
			VkPipelineViewportStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			createInfo.viewportCount = _viewportCount;
			createInfo.pViewports = _viewports;
			createInfo.scissorCount = _scissorCount;
			createInfo.pScissors = _scissors;
			createInfo.flags = _flags;
			return createInfo;
		}

		inline VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(
			VkPolygonMode _polygonMode = VK_POLYGON_MODE_FILL,
			VkCullModeFlagBits _cullMode = VK_CULL_MODE_BACK_BIT,
			VkFrontFace _frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			VkPipelineRasterizationStateCreateFlags _flags = 0
			)
		{
			VkPipelineRasterizationStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			createInfo.frontFace = _frontFace;
			createInfo.cullMode = _cullMode;
			createInfo.polygonMode = _polygonMode;
			createInfo.lineWidth = 1.0f;
			createInfo.depthBiasEnable = VK_FALSE;
			createInfo.depthClampEnable = VK_FALSE;
			createInfo.rasterizerDiscardEnable = VK_FALSE;
			createInfo.flags = _flags;
			return createInfo;
		}

		inline VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo(
			VkSampleCountFlagBits _sampleCount = VK_SAMPLE_COUNT_1_BIT,
			VkPipelineMultisampleStateCreateFlags _flags = 0
			)
		{
			VkPipelineMultisampleStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			createInfo.rasterizationSamples = _sampleCount;
			createInfo.flags = _flags;
			return createInfo;
		}

		inline VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(
			VkBool32 _depthTestEnable,
			VkBool32 _depthWriteEnable,
			VkCompareOp _compareOp
			)
		{
			VkPipelineDepthStencilStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			createInfo.depthTestEnable = _depthTestEnable;
			createInfo.depthWriteEnable = _depthWriteEnable;
			createInfo.depthCompareOp = _compareOp;
			createInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			createInfo.depthBoundsTestEnable = VK_FALSE;
			return createInfo;
		}

		inline VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(
			VkBool32 _blendEnable,
			VkColorComponentFlags _colorWriteMask
			)
		{
			VkPipelineColorBlendAttachmentState createInfo = {};
			createInfo.blendEnable = _blendEnable;
			createInfo.colorWriteMask = _colorWriteMask;
			return createInfo;
		}

		inline VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(
			uint32_t _attachmentCount,
			const VkPipelineColorBlendAttachmentState* _attachments
			)
		{
			VkPipelineColorBlendStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			createInfo.attachmentCount = _attachmentCount;
			createInfo.pAttachments = _attachments;
			//createInfo.logicOpEnable = VK_FALSE;
			return createInfo;
		}

		inline VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(
			uint32_t _stateCount,
			const VkDynamicState* _states,
			VkPipelineDynamicStateCreateFlags _flags = 0
			)
		{
			VkPipelineDynamicStateCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			createInfo.dynamicStateCount = _stateCount;
			createInfo.pDynamicStates = _states;
			createInfo.flags = _flags;
			return createInfo;
		}

		inline VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(
			VkShaderStageFlagBits _stage,
			VkShaderModule _module,
			const char* _entryPoint = "main"
			)
		{
			VkPipelineShaderStageCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			createInfo.stage = _stage;
			createInfo.module = _module;
			createInfo.pName = _entryPoint;
			return createInfo;
		}

		inline VkSwapchainCreateInfoKHR SwapchainCreateInfo(
			VkSurfaceFormatKHR _format,
			VkPresentModeKHR _presentMode,
			VkExtent2D _imageExtent,
			VkSurfaceKHR _surface,
			uint32_t _minImageCount,
			VkSurfaceTransformFlagBitsKHR _transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			uint32_t _arrayLayers = 1
			)
		{
			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.imageFormat = _format.format;
			createInfo.imageColorSpace = _format.colorSpace;
			createInfo.presentMode = _presentMode;
			createInfo.imageExtent = _imageExtent;
			createInfo.preTransform = _transform;
			createInfo.minImageCount = _minImageCount;
			createInfo.surface = _surface;
			createInfo.clipped = VK_TRUE;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.imageArrayLayers = _arrayLayers;
			createInfo.oldSwapchain = VK_NULL_HANDLE;
			return createInfo;
		}


	}
}

