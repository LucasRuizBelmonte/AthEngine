/**
 * @file Collider2D.h
 * @brief Declarations for Collider2D.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include <cstdint>
#pragma endregion

#pragma region Declarations
struct Collider2D
{
	enum class Shape : uint8_t
	{
		AABB = 0,
		Circle
	};

	Shape shape = Shape::AABB;
	bool isTrigger = false;
	uint32_t layer = 1u;
	uint32_t mask = 0xFFFFFFFFu;

	glm::vec2 halfExtents{0.5f, 0.5f};
	float radius = 0.5f;
	glm::vec2 offset{0.f, 0.f};
};
#pragma endregion
