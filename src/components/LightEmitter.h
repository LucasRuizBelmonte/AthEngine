/**
 * @file LightEmitter.h
 * @brief Declarations for LightEmitter.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
enum class LightType : uint8_t
{
	Directional = 0,
	Point = 1,
	Spot = 2
};

struct LightEmitter
{
	LightType type = LightType::Directional;

	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float intensity = 1.0f;

	float range = 10.0f;

	float innerCone = 0.5f;
	float outerCone = 0.7f;

	bool castShadows = false;
};
#pragma endregion
