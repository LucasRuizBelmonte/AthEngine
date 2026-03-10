#pragma region Includes
#include "TriggerZoneConsoleSystem.h"

#include "../components/Tag.h"
#include "../components/Transform.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/Physics2D.h"
#include "../physics2d/Physics2DAlgorithms.h"
#include "../utils/Console.h"

#include <algorithm>
#include <string>
#pragma endregion

#pragma region Function Definitions
namespace
{
	static bool IsTriggerZoneTagName(const std::string &tagName)
	{
		return tagName == "Trigger_Zone" || tagName == "Trigger-Zone";
	}

	static bool IsTriggerZoneEntity(const Registry &registry, Entity entity)
	{
		if (!registry.IsAlive(entity) || !registry.Has<Tag>(entity))
			return false;

		return IsTriggerZoneTagName(registry.Get<Tag>(entity).name);
	}

	static const char *TriggerPhaseLabel(events::PhysicsEventPhase2D phase)
	{
		switch (phase)
		{
		case events::PhysicsEventPhase2D::Enter:
			return "Enter";
		case events::PhysicsEventPhase2D::Stay:
			return "Stay";
		case events::PhysicsEventPhase2D::Exit:
			return "Exit";
		}
		return "Unknown";
	}

	static std::string EntityDebugName(const Registry &registry, Entity entity)
	{
		if (registry.IsAlive(entity) && registry.Has<Tag>(entity))
			return registry.Get<Tag>(entity).name;

		return "Entity " + std::to_string(EntityIdOf(entity));
	}

	static bool TryGetEntityWorldShape(const Registry &registry,
	                                   Entity entity,
	                                   const Collider2D *&outCollider,
	                                   Physics2DAlgorithms::WorldShape2D &outShape)
	{
		outCollider = nullptr;
		if (!registry.IsAlive(entity) ||
		    !registry.Has<Transform>(entity) ||
		    !registry.Has<Collider2D>(entity))
			return false;

		const Transform &transform = registry.Get<Transform>(entity);
		const Collider2D &collider = registry.Get<Collider2D>(entity);
		outCollider = &collider;
		outShape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
		return true;
	}

	static std::string HitSummary(const Registry &registry,
	                              const char *queryName,
	                              bool hit,
	                              const Physics2D::QueryHit2D &queryHit)
	{
		if (!hit)
			return std::string(queryName) + ": none";

		return std::string(queryName) + ": " + EntityDebugName(registry, queryHit.entity) +
		       " (d=" + std::to_string(queryHit.distance) + ")";
	}
}

void TriggerZoneConsoleSystem::Update(const Registry &registry, const events::SceneEventBus &eventBus) const
{
	const std::vector<events::PhysicsTriggerEvent2D> &triggerEvents = eventBus.Get<events::PhysicsTriggerEvent2D>();
	for (const events::PhysicsTriggerEvent2D &triggerEvent : triggerEvents)
	{
		const bool triggerZoneIsA = IsTriggerZoneEntity(registry, triggerEvent.a);
		const bool triggerZoneIsB = IsTriggerZoneEntity(registry, triggerEvent.b);
		if (!triggerZoneIsA && !triggerZoneIsB)
			continue;

		const Entity other = triggerZoneIsA ? triggerEvent.b : triggerEvent.a;
		Console::Print("Trigger-Zone " + std::string(TriggerPhaseLabel(triggerEvent.phase)) +
		                   ": " + EntityDebugName(registry, other),
		               Info);

		if (triggerEvent.phase != events::PhysicsEventPhase2D::Enter)
			continue;

		const Entity triggerZone = triggerZoneIsA ? triggerEvent.a : triggerEvent.b;
		const Collider2D *triggerZoneCollider = nullptr;
		Physics2DAlgorithms::WorldShape2D triggerZoneShape;
		if (!TryGetEntityWorldShape(registry, triggerZone, triggerZoneCollider, triggerZoneShape))
			continue;

		Physics2D::QueryFilter2D queryFilter;
		queryFilter.layerMask = 0xFFFFFFFFu;
		queryFilter.queryLayer = triggerZoneCollider->layer;
		queryFilter.queryMask = triggerZoneCollider->mask;
		queryFilter.respectCollisionMatrix = true;
		queryFilter.includeTriggers = false;
		queryFilter.ignoreEntity = triggerZone;

		std::vector<Entity> overlaps;
		if (triggerZoneShape.shape == Collider2D::Shape::Circle)
		{
			overlaps = Physics2D::OverlapCircle(registry,
			                                    triggerZoneShape.center,
			                                    triggerZoneShape.radius,
			                                    queryFilter);
		}
		else
		{
			overlaps = Physics2D::OverlapBox(registry,
			                                 triggerZoneShape.center,
			                                 triggerZoneShape.halfExtents,
			                                 triggerZoneShape.rotation,
			                                 queryFilter);
		}

		Physics2D::RaycastHit2D raycastHit;
		const bool hasRaycastHit =
			Physics2D::Raycast(registry,
			                   triggerZoneShape.center,
			                   glm::vec2(0.f, 1.f),
			                   12.f,
			                   raycastHit,
			                   queryFilter);

		Physics2D::ShapeCastHit2D shapeCastHit;
		bool hasShapeCastHit = false;
		if (triggerZoneShape.shape == Collider2D::Shape::Circle)
		{
			const float castRadius = std::max(0.1f, triggerZoneShape.radius * 0.5f);
			hasShapeCastHit = Physics2D::CircleCast(registry,
			                                        triggerZoneShape.center,
			                                        castRadius,
			                                        glm::vec2(0.f, 1.f),
			                                        6.f,
			                                        shapeCastHit,
			                                        queryFilter);
		}
		else
		{
			const glm::vec2 castHalfExtents(std::max(0.1f, triggerZoneShape.halfExtents.x * 0.5f),
			                                std::max(0.1f, triggerZoneShape.halfExtents.y * 0.5f));
			hasShapeCastHit = Physics2D::BoxCast(registry,
			                                     triggerZoneShape.center,
			                                     castHalfExtents,
			                                     triggerZoneShape.rotation,
			                                     glm::vec2(0.f, 1.f),
			                                     6.f,
			                                     shapeCastHit,
			                                     queryFilter);
		}

		Console::Print("Physics2D Query Snapshot (" + EntityDebugName(registry, triggerZone) +
		                   ") overlaps=" + std::to_string(overlaps.size()) + ", " +
		                   HitSummary(registry, "ray", hasRaycastHit, raycastHit) + ", " +
		                   HitSummary(registry, "cast", hasShapeCastHit, shapeCastHit),
		               Info);
	}
}
#pragma endregion
