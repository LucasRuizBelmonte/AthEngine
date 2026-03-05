#pragma region Includes
#include "Physics2D.h"

#include "Collider2D.h"
#include "PhysicsBody2D.h"
#include "Physics2DAlgorithms.h"

#include "../components/Transform.h"

#include <algorithm>
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

		static std::vector<Entity> OverlapInternal(const Registry &registry,
		                                           const Physics2DAlgorithms::WorldShape2D &queryShape,
		                                           uint32_t layerMask)
		{
			std::vector<Entity> candidates;
			registry.ViewEntities<Transform, Collider2D>(candidates);

			std::vector<Entity> results;
			results.reserve(candidates.size());

			for (Entity entity : candidates)
			{
				if (!IsPhysicsEnabled(registry, entity))
					continue;

				const Collider2D &collider = registry.Get<Collider2D>(entity);
				if ((collider.layer & layerMask) == 0u)
					continue;

				const Transform &transform = registry.Get<Transform>(entity);
				const Physics2DAlgorithms::WorldShape2D bodyShape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
				Physics2DAlgorithms::Contact2D contact;
				if (Physics2DAlgorithms::TestOverlap(bodyShape, queryShape, contact))
					results.push_back(entity);
			}

			return results;
		}
	}

	std::vector<Entity> OverlapCircle(const Registry &registry,
	                                  const glm::vec2 &center,
	                                  float radius,
	                                  uint32_t layerMask)
	{
		Physics2DAlgorithms::WorldShape2D queryShape;
		queryShape.shape = Collider2D::Shape::Circle;
		queryShape.center = center;
		queryShape.radius = std::max(0.f, radius);
		return OverlapInternal(registry, queryShape, layerMask);
	}

	std::vector<Entity> OverlapAABB(const Registry &registry,
	                                const glm::vec2 &center,
	                                const glm::vec2 &halfExtents,
	                                uint32_t layerMask)
	{
		Physics2DAlgorithms::WorldShape2D queryShape;
		queryShape.shape = Collider2D::Shape::AABB;
		queryShape.center = center;
		queryShape.halfExtents = glm::max(glm::vec2(0.f), halfExtents);
		return OverlapInternal(registry, queryShape, layerMask);
	}
}
#pragma endregion
