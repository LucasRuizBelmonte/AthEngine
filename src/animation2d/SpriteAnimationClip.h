/**
 * @file SpriteAnimationClip.h
 * @brief Declarations for SpriteAnimationClip.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include <vector>
#pragma endregion

#pragma region Declarations
struct SpriteAnimationClip
{
	std::vector<glm::vec4> framesUV;
	float fps = 12.f;
	bool loop = true;
};
#pragma endregion
