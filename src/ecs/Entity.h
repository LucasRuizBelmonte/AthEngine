/**
 * @file Entity.h
 * @brief Declarations for Entity.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#pragma endregion

#pragma region Declarations
using Entity = uint32_t;
static constexpr Entity kInvalidEntity = 0xFFFFFFFFu;

static constexpr uint32_t kEntityIdBits = 20u;
static constexpr uint32_t kEntityGenerationBits = 12u;

static constexpr uint32_t kEntityIdMask = (1u << kEntityIdBits) - 1u;
static constexpr uint32_t kEntityGenerationMask = (1u << kEntityGenerationBits) - 1u;

inline constexpr uint32_t EntityIdOf(Entity entity)
{
	return entity & kEntityIdMask;
}

inline constexpr uint32_t EntityGenerationOf(Entity entity)
{
	return (entity >> kEntityIdBits) & kEntityGenerationMask;
}

inline constexpr Entity MakeEntity(uint32_t id, uint32_t generation)
{
	return ((generation & kEntityGenerationMask) << kEntityIdBits) | (id & kEntityIdMask);
}
#pragma endregion
