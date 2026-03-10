/**
 * @file Physics2D.h
 * @brief Declarations for Physics2D query helpers.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#include "Collider2D.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#pragma endregion

#pragma region Declarations
namespace Physics2D
{
	struct QueryFilter2D
	{
		uint32_t layerMask = 0xFFFFFFFFu;
		uint32_t queryCollisionLayer = Physics2DCollisionFiltering::kDefaultCollisionLayer;
		uint32_t queryCollisionMask = Physics2DCollisionFiltering::kDefaultCollisionMask;
		bool respectCollisionMatrix = true;
		bool includeTriggers = true;
		bool includeDisabled = false;
		Entity ignoreEntity = kInvalidEntity;
	};

	struct QueryHit2D
	{
		Entity entity = kInvalidEntity;
		glm::vec2 point{0.f, 0.f};
		glm::vec2 normal{0.f, 1.f};
		float distance = 0.f;
		float fraction = 0.f;
	};

	using RaycastHit2D = QueryHit2D;
	using ShapeCastHit2D = QueryHit2D;

	bool Raycast(const Registry &registry,
	             const glm::vec2 &origin,
	             const glm::vec2 &direction,
	             float maxDistance,
	             RaycastHit2D &outHit,
	             const QueryFilter2D &filter = QueryFilter2D{});

	std::vector<RaycastHit2D> RaycastAll(const Registry &registry,
	                                     const glm::vec2 &origin,
	                                     const glm::vec2 &direction,
	                                     float maxDistance,
	                                     const QueryFilter2D &filter = QueryFilter2D{});

	std::vector<Entity> OverlapPoint(const Registry &registry,
	                                 const glm::vec2 &point,
	                                 const QueryFilter2D &filter = QueryFilter2D{});

	std::vector<Entity> OverlapCircle(const Registry &registry,
	                                  const glm::vec2 &center,
	                                  float radius,
	                                  const QueryFilter2D &filter);

	std::vector<Entity> OverlapBox(const Registry &registry,
	                               const glm::vec2 &center,
	                               const glm::vec2 &halfExtents,
	                               float rotationRadians,
	                               const QueryFilter2D &filter = QueryFilter2D{});

	bool CircleCast(const Registry &registry,
	                const glm::vec2 &origin,
	                float radius,
	                const glm::vec2 &direction,
	                float maxDistance,
	                ShapeCastHit2D &outHit,
	                const QueryFilter2D &filter = QueryFilter2D{});

	bool BoxCast(const Registry &registry,
	             const glm::vec2 &origin,
	             const glm::vec2 &halfExtents,
	             float rotationRadians,
	             const glm::vec2 &direction,
	             float maxDistance,
	             ShapeCastHit2D &outHit,
	             const QueryFilter2D &filter = QueryFilter2D{});

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
