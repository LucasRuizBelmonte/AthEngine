#pragma region Includes
#include "GameplayEventProjectionSystem.h"

#include "../components/runtime/RuntimeResources.h"
#pragma endregion

#pragma region Function Definitions
void GameplayEventProjectionSystem::Reset(Registry &registry) const
{
	GameplayEventProjectionState &state = registry.EnsureResource<GameplayEventProjectionState>();
	state.hasProjectedStep = false;
	state.lastProjectedStep = 0u;
}

void GameplayEventProjectionSystem::Update(Registry &registry, events::SceneEventBus &eventBus) const
{
	RuntimeSimulationClock &clock = registry.EnsureResource<RuntimeSimulationClock>();
	GameplayEventProjectionState &state = registry.EnsureResource<GameplayEventProjectionState>();

	eventBus.Clear<events::GameplayEvent>();

	if (state.hasProjectedStep && state.lastProjectedStep == clock.fixedStepCounter)
		return;

	state.hasProjectedStep = true;
	state.lastProjectedStep = clock.fixedStepCounter;

	const std::vector<events::PhysicsTriggerEvent2D> &triggerEvents = eventBus.Get<events::PhysicsTriggerEvent2D>();
	for (const events::PhysicsTriggerEvent2D &triggerEvent : triggerEvents)
	{
		if (triggerEvent.phase != events::PhysicsEventPhase2D::Enter)
			continue;

		events::GameplayEvent gameplayEvent{};
		gameplayEvent.type = events::GameplayEventType::TriggerEnter2D;
		gameplayEvent.source = triggerEvent.a;
		gameplayEvent.target = triggerEvent.b;
		gameplayEvent.value = triggerEvent.penetration;
		eventBus.Push(gameplayEvent);
	}
}
#pragma endregion
