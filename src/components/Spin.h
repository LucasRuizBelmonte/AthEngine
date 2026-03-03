/**
 * @file Spin.h
 * @brief Declarations for Spin.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct Spin
{
	glm::vec3 axis{0.f, 0.f, -1.f};
	float freq = 0.8f;
	float amplitude = 0.25f;
};
#pragma endregion
