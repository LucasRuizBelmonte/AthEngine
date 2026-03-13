/**
 * @file UISprite.h
 * @brief Runtime UI sprite component.
 */

#pragma once

#pragma region Includes
#include "../../resources/ResourceHandle.h"

#include <glm/glm.hpp>
#include <string>
#pragma endregion

#pragma region Declarations
class Texture;
class Shader;

struct UISprite
{
	ResourceHandle<Texture> texture;
	ResourceHandle<Shader> shader;
	std::string texturePath;
	std::string materialPath = "shaders/unlit2D.fs";

	glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
	glm::vec4 uv{0.0f, 0.0f, 1.0f, 1.0f};
	int layer = 0;
	int orderInLayer = 0;
	bool preserveAspect = false;
};
#pragma endregion
