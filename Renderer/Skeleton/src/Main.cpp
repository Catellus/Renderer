
#include <iostream>

#include "Common.h"

#include "SkeletonApplication.h"
#include "Lights.h"
#include "Object.h"

class Application : public skel::SkeletonApplication
{
private:
	VulkanDevice* device;
	std::vector<std::vector<Object*>*> renderableObjects;
	std::vector<Object*> bulbs;
	std::vector<Object*> subjects;

	skel::lights::ShaderLights finalLights;

	const char* albedoTextureDir    = "TestTextures\\White_albedo.png";
	const char* normalTextureDir    = "TestTextures\\EggCartonFoam_normal.png";
	const char* metallicTextureDir  = "TestTextures\\Black_metallic.png";
	const char* roughnessTextureDir = "TestTextures\\Black_roughness.png";
	const char* aoTextureDir        = "TestTextures\\Black_ao.png";

protected:
	void ChildInitialize()
	{
		device = renderer->device;

		bulbs.resize(4);
		subjects.resize(4);

		BindShaderDescriptors();

		#pragma region LightInfo
		glm::vec3 directionalColor = { 1.0f, 0.0f, 0.0f };

		glm::vec3 trPointColor = { 1.0f, 1.0f, 1.0f };
		glm::vec3 tlPointColor = { 0.0f, 1.0f, 1.0f };
		glm::vec3 blPointColor = { 1.0f, 0.0f, 1.0f };
		glm::vec3 brPointColor = { 1.0f, 1.0f, 0.0f };

		glm::vec3 spotlightColor1 = { 203.f / 255.f, 213.f / 255.f, 219.f / 255.f };
		glm::vec3 spotlightColor2 = { 0 / 255.f, 0 / 255.f, 0 / 255.f };

		// Define lighting
		// Directional light
		finalLights.directionalLight.color = directionalColor * 1.f;
		finalLights.directionalLight.direction = { 0.0f, -1.0f, 0.1f };
		// Point lights
		finalLights.pointLights[0] = { trPointColor * 2.0f, {  1.0f,  1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		finalLights.pointLights[1] = { tlPointColor * 2.0f, { -1.0f,  1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		finalLights.pointLights[2] = { blPointColor * 2.0f, { -1.0f, -1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		finalLights.pointLights[3] = { brPointColor * 2.0f, {  1.0f, -1.0f, 2.0f }, { 1.0f, 0.35f, 0.44f } };
		// Spot lights
		finalLights.spotLights[0].color = spotlightColor1 * 0.f;
		finalLights.spotLights[0].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[0].cutOff = glm::cos(glm::radians(1.0f));
		finalLights.spotLights[0].outerCutOff = glm::cos(glm::radians(7.0f));

		finalLights.spotLights[1].color = spotlightColor2 * 0.f;
		finalLights.spotLights[1].CLQ = { 1.0f, 0.35f, 0.44f };
		finalLights.spotLights[1].cutOff = glm::cos(glm::radians(2.0f));
		finalLights.spotLights[1].outerCutOff = glm::cos(glm::radians(3.0f));

		#pragma endregion

		uint32_t index = 0;
		for (auto& object : bulbs)
		{
			object = new Object(device, skel::shaders::ShaderTypes::Unlit, ".\\res\\models\\TestShapes\\SphereSmooth.obj");
			object->AttachBuffer(sizeof(glm::vec3));
			VkDeviceMemory* bulbColorMemory = &object->shader.buffers[1]->memory;
			renderer->shaderDescriptors[0]->CreateDescriptorSets(device->logicalDevice, object->shader);
			object->transform.position = finalLights.pointLights[index].position;
			object->transform.scale *= 0.05f;
			device->CopyDataToBufferMemory(&finalLights.pointLights[index].color, sizeof(glm::vec3), *bulbColorMemory);

			index++;
		}

		renderableObjects.push_back(&bulbs);
		renderer->RecordRenderingCommandBuffers(&renderableObjects);
	}

	void BindShaderDescriptors()
	{
		// Bind shader descriptors
		renderer->AddShader(
			"unlit",
			{
				// MVP matrices
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
				// Object color
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			},
			4
			);
		renderer->AddShader(
			"PBR",
			{
				// MVP matrices
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
				// Lights info
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
				// Albedo map
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
				// Normal map
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
				// Metallic
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
				// Roughness
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
				// AO
				skel::initializers::DescriptorSetLyoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
			},
			4
			);
	}

	void MainLoopCore()
	{
		static bool addedSubjects = false;

		// TODO : Create an input manager
		HandleInput();

		UpdateObjectUniforms();

		if (!addedSubjects && time.totalTime > 1.0f)
		{
			addedSubjects = true;

			uint32_t index = 0;

			for (auto& object : subjects)
			{
				object = new Object(device, skel::shaders::ShaderTypes::Opaque, ".\\res\\models\\TestShapes\\Cube.obj");
				object->AttachBuffer(sizeof(finalLights));
				VkDeviceMemory* lightsMemory = &object->shader.buffers[1]->memory;
				object->AttachTexture(albedoTextureDir);
				object->AttachTexture(normalTextureDir);
				object->AttachTexture(metallicTextureDir);
				object->AttachTexture(roughnessTextureDir);
				object->AttachTexture(aoTextureDir);
				renderer->shaderDescriptors[1]->CreateDescriptorSets(device->logicalDevice, object->shader);

				object->transform.position = {index & 1 ? 1.0f : -1.0f, index & 2 ? 1.0f : -1.0f, 0.0f};
				object->transform.scale *= 0.99f;

				device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), *lightsMemory);
				index++;
			}

			renderableObjects.push_back(&subjects);
			renderer->RecordRenderingCommandBuffers(&renderableObjects);
		}

		renderer->RenderFrame();
	}

	void UpdateObjectUniforms()
	{
		finalLights.spotLights[0].position = cam->cameraPosition;
		finalLights.spotLights[0].direction = cam->cameraFront;

		finalLights.spotLights[1].position = cam->cameraPosition;
		finalLights.spotLights[1].direction = cam->cameraFront;

		for (auto& object : bulbs)
		{
			object->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
		}

		if (static_cast<uint32_t>(renderableObjects.size()) > 1)
		{
			for (auto& object : subjects)
			{
				device->CopyDataToBufferMemory(&finalLights, sizeof(finalLights), object->shader.buffers[1]->memory);
				object->UpdateMVPBuffer(cam->cameraPosition, cam->projection, cam->view);
			}
		}
	}

	void ChildCleanup()
	{
		for (const auto& object : bulbs)
		{
			free(object);
		}

		for (const auto& object : subjects)
		{
			free(object);
		}
	}

};

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	try
	{
		Application app;
		app.Run();
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
		_CrtDumpMemoryLeaks();
		return EXIT_FAILURE;
	}
	_CrtDumpMemoryLeaks();
	return EXIT_SUCCESS;
}

