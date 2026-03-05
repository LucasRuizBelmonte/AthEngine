/**
 * @file Physics2D.h
 * @brief Declarations for Physics2D overlap queries.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#pragma endregion

#pragma region Declarations
namespace Physics2D
{
	std::vector<Entity> OverlapCircle(const Registry &registry,
	                                  const glm::vec2 &center,
	                                  float radius,
	                                  uint32_t layerMask = 0xFFFFFFFFu);

	std::vector<Entity> OverlapAABB(const Registry &registry,
	                                const glm::vec2 &center,
	                                const glm::vec2 &halfExtents,
	                                uint32_t layerMask = 0xFFFFFFFFu);
}
#pragma endregion
