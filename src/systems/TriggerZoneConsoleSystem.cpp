#pragma region Includes
#include "TriggerZoneConsoleSystem.h"

#include "../components/Tag.h"
#include "../utils/Console.h"
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
	}
}
#pragma endregion
