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
namespace Physics2DCollisionFiltering
{
	constexpr uint32_t kLayerCount = 32u;
	constexpr uint32_t kDefaultCollisionLayer = 1u;
	constexpr uint32_t kDefaultCollisionMask = 0xFFFFFFFFu;

	constexpr uint32_t LayerBitFromIndex(uint32_t layerIndex)
	{
		return (layerIndex < kLayerCount) ? (1u << layerIndex) : 0u;
	}

	constexpr bool MaskContainsLayer(uint32_t mask, uint32_t layerBits)
	{
		return (mask & layerBits) != 0u;
	}

	inline void SetLayerBit(uint32_t &mask, uint32_t layerBit, bool enabled)
	{
		mask = enabled ? (mask | layerBit) : (mask & ~layerBit);
	}
}

struct Collider2D
{
	enum class Shape : uint8_t
	{
		AABB = 0,
		Circle
	};

	Shape shape = Shape::AABB;
	bool isTrigger = false;
	uint32_t collisionLayer = Physics2DCollisionFiltering::kDefaultCollisionLayer;
	uint32_t collisionMask = Physics2DCollisionFiltering::kDefaultCollisionMask;

	glm::vec2 halfExtents{0.5f, 0.5f};
	float radius = 0.5f;
	glm::vec2 offset{0.f, 0.f};
};
#pragma endregion
