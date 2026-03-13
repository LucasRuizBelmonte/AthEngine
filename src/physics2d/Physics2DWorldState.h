/**
 * @file Physics2DWorldState.h
 * @brief ECS resource state for the 2D physics world.
 */

#pragma once

#pragma region Includes
#include "../ecs/Entity.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region Declarations
struct Physics2DCollisionPairKey
{
	Entity a = kInvalidEntity;
	Entity b = kInvalidEntity;
	bool trigger = false;

	bool operator==(const Physics2DCollisionPairKey &other) const
	{
		return a == other.a && b == other.b && trigger == other.trigger;
	}
};

struct Physics2DCollisionPairKeyHash
{
	size_t operator()(const Physics2DCollisionPairKey &key) const
	{
		const size_t h1 = std::hash<uint32_t>{}(key.a);
		const size_t h2 = std::hash<uint32_t>{}(key.b);
		const size_t h3 = std::hash<uint8_t>{}(key.trigger ? 1u : 0u);
		return h1 ^ (h2 << 1u) ^ (h3 << 2u);
	}
};

struct Physics2DCollisionPairState
{
	glm::vec2 normal{0.f, 0.f};
	float penetration = 0.f;
};

struct Physics2DStepStats
{
	size_t bodyCount = 0u;
	size_t broadphasePairCount = 0u;
	size_t narrowphasePairCount = 0u;
	size_t contactCount = 0u;
};

struct Physics2DWorldState
{
	std::unordered_map<Physics2DCollisionPairKey, Physics2DCollisionPairState, Physics2DCollisionPairKeyHash> previousPairs;
	std::unordered_map<Physics2DCollisionPairKey, Physics2DCollisionPairState, Physics2DCollisionPairKeyHash> currentPairs;
	std::vector<Entity> dynamicEntities;
	std::vector<Entity> colliderEntities;
	Physics2DStepStats lastStepStats{};
	glm::vec2 gravity{0.f, -9.81f};
};
#pragma endregion
