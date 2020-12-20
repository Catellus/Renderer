#pragma once

#include <vulkan/vulkan.h>

#include <sdl/SDL.h>
#include <sdl/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED // Sort of breaks GLSL (ALl Zs need to be flipped)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// Resources
static const char* resourcePrefix = ".\\res\\";
static const char* shaderPrefix = ".\\res\\shaders\\";
static const char* texturePrefix = ".\\res\\textures\\";
static const char* modelPrefix = ".\\res\\models\\";


