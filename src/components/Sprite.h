/**
 * @file Sprite.h
 * @brief Declarations for Sprite.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include "../resources/ResourceHandle.h"
#pragma endregion

#pragma region Declarations
class Texture;
class Shader;

struct Sprite
{
	ResourceHandle<Texture> texture;
	ResourceHandle<Shader> shader;

	glm::vec2 size{1.f, 1.f};
	glm::vec4 uv{0.f, 0.f, 1.f, 1.f};
	glm::vec4 tint{1.f, 1.f, 1.f, 1.f};

	int layer = 0;
	int orderInLayer = 0;
};
#pragma endregion
