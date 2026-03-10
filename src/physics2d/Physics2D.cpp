#pragma region Includes
#include "Physics2D.h"

#include "Collider2D.h"
#include "PhysicsBody2D.h"
#include "Physics2DAlgorithms.h"

#include "../components/Transform.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#pragma endregion

#pragma region Function Definitions
namespace Physics2D
{
	namespace
	{
		static bool IsPhysicsEnabled(const Registry &registry, Entity entity)
		{
			return !registry.Has<PhysicsBody2D>(entity) || registry.Get<PhysicsBody2D>(entity).enabled;
		}

		static bool PassesFilter(const Registry &registry,
		                         Entity entity,
		                         const Collider2D &collider,
		                         const QueryFilter2D &filter)
		{
			if (entity == filter.ignoreEntity)
				return false;

			if (!filter.includeDisabled && !IsPhysicsEnabled(registry, entity))
				return false;

			if (!filter.includeTriggers && collider.isTrigger)
				return false;

			if ((collider.collisionLayer & filter.layerMask) == 0u)
				return false;

			if (filter.respectCollisionMatrix)
			{
				if ((filter.queryCollisionMask & collider.collisionLayer) == 0u)
					return false;
				if ((collider.collisionMask & filter.queryCollisionLayer) == 0u)
					return false;
			}

			return true;
		}

		static Physics2DAlgorithms::WorldShape2D BuildCircleShape(const glm::vec2 &center, float radius)
		{
			Physics2DAlgorithms::WorldShape2D shape;
			shape.shape = Collider2D::Shape::Circle;
			shape.center = center;
			shape.radius = std::max(0.f, radius);
			return shape;
		}

		static Physics2DAlgorithms::WorldShape2D BuildBoxShape(const glm::vec2 &center,
		                                                       const glm::vec2 &halfExtents,
		                                                       float rotationRadians)
		{
			Physics2DAlgorithms::WorldShape2D shape;
			shape.shape = Collider2D::Shape::AABB;
			shape.center = center;
			shape.halfExtents = glm::max(glm::vec2(0.f), halfExtents);
			shape.rotation = rotationRadians;
			shape.axisX = Physics2DAlgorithms::RotateOffset(glm::vec2(1.f, 0.f), rotationRadians);
			shape.axisY = Physics2DAlgorithms::RotateOffset(glm::vec2(0.f, 1.f), rotationRadians);
			return shape;
		}

		static bool IsDirectionValid(const glm::vec2 &direction, glm::vec2 &outDirectionNormalized)
		{
			const float lenSq = glm::dot(direction, direction);
			if (lenSq <= 0.000001f)
			{
				outDirectionNormalized = glm::vec2(0.f, 0.f);
				return false;
			}

			outDirectionNormalized = direction / std::sqrt(lenSq);
			return true;
		}

		static bool IsBetterHit(const QueryHit2D &candidate, const QueryHit2D &best)
		{
			if (candidate.distance < best.distance - 0.00001f)
				return true;
			if (std::abs(candidate.distance - best.distance) <= 0.00001f)
				return candidate.entity < best.entity;
			return false;
		}

		static bool IsRayBroadphaseCandidate(const glm::vec2 &origin,
		                                     const glm::vec2 &directionNormalized,
		                                     float maxDistance,
		                                     const Physics2DAlgorithms::WorldShape2D &shape)
		{
			const glm::vec2 end = origin + directionNormalized * maxDistance;
			const float broadphaseRadius = Physics2DAlgorithms::ShapeBroadphaseRadius(shape);
			return Physics2DAlgorithms::DistanceSqPointSegment(shape.center, origin, end) <=
			       broadphaseRadius * broadphaseRadius;
		}

		static bool IsSweepBroadphaseCandidate(const Physics2DAlgorithms::WorldShape2D &movingShape,
		                                       const glm::vec2 &travelDelta,
		                                       const Physics2DAlgorithms::WorldShape2D &targetShape)
		{
			const glm::vec2 start = movingShape.center;
			const glm::vec2 end = movingShape.center + travelDelta;
			const float radiusSum =
				Physics2DAlgorithms::ShapeBroadphaseRadius(movingShape) +
				Physics2DAlgorithms::ShapeBroadphaseRadius(targetShape);
			return Physics2DAlgorithms::DistanceSqPointSegment(targetShape.center, start, end) <=
			       radiusSum * radiusSum;
		}

		static std::vector<Entity> OverlapInternal(const Registry &registry,
		                                           const Physics2DAlgorithms::WorldShape2D &queryShape,
		                                           const QueryFilter2D &filter)
		{
			std::vector<Entity> candidates;
			registry.ViewEntities<Transform, Collider2D>(candidates);

			std::vector<Entity> results;
			results.reserve(candidates.size());
			const float queryBroadphaseRadius = Physics2DAlgorithms::ShapeBroadphaseRadius(queryShape);

			for (Entity entity : candidates)
			{
				const Collider2D &collider = registry.Get<Collider2D>(entity);
				if (!PassesFilter(registry, entity, collider, filter))
					continue;

				const Transform &transform = registry.Get<Transform>(entity);
				const Physics2DAlgorithms::WorldShape2D bodyShape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
				const float combinedRadius = queryBroadphaseRadius + Physics2DAlgorithms::ShapeBroadphaseRadius(bodyShape);
				const glm::vec2 centerDelta = bodyShape.center - queryShape.center;
				if (glm::dot(centerDelta, centerDelta) > combinedRadius * combinedRadius)
					continue;

				Physics2DAlgorithms::Contact2D contact;
				if (Physics2DAlgorithms::TestOverlap(bodyShape, queryShape, contact))
					results.push_back(entity);
			}

			std::sort(results.begin(), results.end());
			return results;
		}

		static bool ShapeCastClosest(const Registry &registry,
		                             const Physics2DAlgorithms::WorldShape2D &movingShape,
		                             const glm::vec2 &direction,
		                             float maxDistance,
		                             ShapeCastHit2D &outHit,
		                             const QueryFilter2D &filter)
		{
			glm::vec2 directionNormalized(0.f, 0.f);
			const bool hasDirection = IsDirectionValid(direction, directionNormalized);
			const float castDistance = std::max(0.f, maxDistance);
			const glm::vec2 travelDelta = (hasDirection && castDistance > 0.f)
			                                  ? directionNormalized * castDistance
			                                  : glm::vec2(0.f, 0.f);

			std::vector<Entity> candidates;
			registry.ViewEntities<Transform, Collider2D>(candidates);

			bool hasBestHit = false;
			QueryHit2D bestHit;
			bestHit.distance = std::numeric_limits<float>::max();
			bestHit.entity = kInvalidEntity;

			for (Entity entity : candidates)
			{
				const Collider2D &collider = registry.Get<Collider2D>(entity);
				if (!PassesFilter(registry, entity, collider, filter))
					continue;

				const Transform &transform = registry.Get<Transform>(entity);
				const Physics2DAlgorithms::WorldShape2D targetShape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
				if (!IsSweepBroadphaseCandidate(movingShape, travelDelta, targetShape))
					continue;

				float fraction = 0.f;
				Physics2DAlgorithms::Contact2D contact;
				if (!Physics2DAlgorithms::SweepOverlapByConservativeSampling(movingShape, travelDelta, targetShape, fraction, contact))
					continue;

				QueryHit2D hit;
				hit.entity = entity;
				hit.fraction = glm::clamp(fraction, 0.f, 1.f);
				hit.distance = castDistance * hit.fraction;
				hit.point = contact.point;
				hit.normal = -contact.normal;
				const float normalLenSq = glm::dot(hit.normal, hit.normal);
				if (normalLenSq <= 0.000001f)
				{
					if (hasDirection)
						hit.normal = -directionNormalized;
					else
						hit.normal = glm::vec2(0.f, 1.f);
				}
				else
				{
					hit.normal /= std::sqrt(normalLenSq);
				}

				if (!hasBestHit || IsBetterHit(hit, bestHit))
				{
					hasBestHit = true;
					bestHit = hit;
				}
			}

			if (!hasBestHit)
				return false;

			outHit = bestHit;
			return true;
		}
	}

	bool Raycast(const Registry &registry,
	             const glm::vec2 &origin,
	             const glm::vec2 &direction,
	             float maxDistance,
	             RaycastHit2D &outHit,
	             const QueryFilter2D &filter)
	{
		glm::vec2 directionNormalized(0.f, 0.f);
		if (!IsDirectionValid(direction, directionNormalized))
			return false;

		const float castDistance = std::max(0.f, maxDistance);
		std::vector<Entity> candidates;
		registry.ViewEntities<Transform, Collider2D>(candidates);

		bool hasBestHit = false;
		QueryHit2D bestHit;
		bestHit.distance = std::numeric_limits<float>::max();
		bestHit.entity = kInvalidEntity;

		for (Entity entity : candidates)
		{
			const Collider2D &collider = registry.Get<Collider2D>(entity);
			if (!PassesFilter(registry, entity, collider, filter))
				continue;

			const Transform &transform = registry.Get<Transform>(entity);
			const Physics2DAlgorithms::WorldShape2D targetShape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
			if (!IsRayBroadphaseCandidate(origin, directionNormalized, castDistance, targetShape))
				continue;

			Physics2DAlgorithms::RaycastShapeHit2D shapeHit;
			if (!Physics2DAlgorithms::RaycastShape(origin, directionNormalized, castDistance, targetShape, shapeHit))
				continue;

			QueryHit2D hit;
			hit.entity = entity;
			hit.point = shapeHit.point;
			hit.normal = shapeHit.normal;
			hit.distance = shapeHit.distance;
			hit.fraction = (castDistance > 0.000001f) ? (shapeHit.distance / castDistance) : 0.f;

			if (!hasBestHit || IsBetterHit(hit, bestHit))
			{
				hasBestHit = true;
				bestHit = hit;
			}
		}

		if (!hasBestHit)
			return false;

		outHit = bestHit;
		return true;
	}

	std::vector<RaycastHit2D> RaycastAll(const Registry &registry,
	                                     const glm::vec2 &origin,
	                                     const glm::vec2 &direction,
	                                     float maxDistance,
	                                     const QueryFilter2D &filter)
	{
		glm::vec2 directionNormalized(0.f, 0.f);
		if (!IsDirectionValid(direction, directionNormalized))
			return {};

		const float castDistance = std::max(0.f, maxDistance);
		std::vector<Entity> candidates;
		registry.ViewEntities<Transform, Collider2D>(candidates);

		std::vector<RaycastHit2D> results;
		results.reserve(candidates.size());
		for (Entity entity : candidates)
		{
			const Collider2D &collider = registry.Get<Collider2D>(entity);
			if (!PassesFilter(registry, entity, collider, filter))
				continue;

			const Transform &transform = registry.Get<Transform>(entity);
			const Physics2DAlgorithms::WorldShape2D targetShape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
			if (!IsRayBroadphaseCandidate(origin, directionNormalized, castDistance, targetShape))
				continue;

			Physics2DAlgorithms::RaycastShapeHit2D shapeHit;
			if (!Physics2DAlgorithms::RaycastShape(origin, directionNormalized, castDistance, targetShape, shapeHit))
				continue;

			RaycastHit2D hit;
			hit.entity = entity;
			hit.point = shapeHit.point;
			hit.normal = shapeHit.normal;
			hit.distance = shapeHit.distance;
			hit.fraction = (castDistance > 0.000001f) ? (shapeHit.distance / castDistance) : 0.f;
			results.push_back(hit);
		}

		std::sort(results.begin(), results.end(), [](const RaycastHit2D &a, const RaycastHit2D &b)
		          {
			          if (a.distance < b.distance - 0.00001f)
				          return true;
			          if (a.distance > b.distance + 0.00001f)
				          return false;
			          return a.entity < b.entity; });
		return results;
	}

	std::vector<Entity> OverlapPoint(const Registry &registry,
	                                 const glm::vec2 &point,
	                                 const QueryFilter2D &filter)
	{
		std::vector<Entity> candidates;
		registry.ViewEntities<Transform, Collider2D>(candidates);

		std::vector<Entity> results;
		results.reserve(candidates.size());
		for (Entity entity : candidates)
		{
			const Collider2D &collider = registry.Get<Collider2D>(entity);
			if (!PassesFilter(registry, entity, collider, filter))
				continue;

			const Transform &transform = registry.Get<Transform>(entity);
			const Physics2DAlgorithms::WorldShape2D bodyShape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
			if (Physics2DAlgorithms::ContainsPoint(bodyShape, point))
				results.push_back(entity);
		}

		std::sort(results.begin(), results.end());
		return results;
	}

	std::vector<Entity> OverlapCircle(const Registry &registry,
	                                  const glm::vec2 &center,
	                                  float radius,
	                                  const QueryFilter2D &filter)
	{
		return OverlapInternal(registry, BuildCircleShape(center, radius), filter);
	}

	std::vector<Entity> OverlapBox(const Registry &registry,
	                               const glm::vec2 &center,
	                               const glm::vec2 &halfExtents,
	                               float rotationRadians,
	                               const QueryFilter2D &filter)
	{
		return OverlapInternal(registry, BuildBoxShape(center, halfExtents, rotationRadians), filter);
	}

	bool CircleCast(const Registry &registry,
	                const glm::vec2 &origin,
	                float radius,
	                const glm::vec2 &direction,
	                float maxDistance,
	                ShapeCastHit2D &outHit,
	                const QueryFilter2D &filter)
	{
		return ShapeCastClosest(registry, BuildCircleShape(origin, radius), direction, maxDistance, outHit, filter);
	}

	bool BoxCast(const Registry &registry,
	             const glm::vec2 &origin,
	             const glm::vec2 &halfExtents,
	             float rotationRadians,
	             const glm::vec2 &direction,
	             float maxDistance,
	             ShapeCastHit2D &outHit,
	             const QueryFilter2D &filter)
	{
		return ShapeCastClosest(registry, BuildBoxShape(origin, halfExtents, rotationRadians), direction, maxDistance, outHit, filter);
	}

	std::vector<Entity> OverlapCircle(const Registry &registry,
	                                  const glm::vec2 &center,
	                                  float radius,
	                                  uint32_t layerMask)
	{
		QueryFilter2D filter;
		filter.layerMask = layerMask;
		filter.respectCollisionMatrix = false;
		return OverlapCircle(registry, center, radius, filter);
	}

	std::vector<Entity> OverlapAABB(const Registry &registry,
	                                const glm::vec2 &center,
	                                const glm::vec2 &halfExtents,
	                                uint32_t layerMask)
	{
		QueryFilter2D filter;
		filter.layerMask = layerMask;
		filter.respectCollisionMatrix = false;
		return OverlapBox(registry, center, halfExtents, 0.f, filter);
	}
}
#pragma endregion
