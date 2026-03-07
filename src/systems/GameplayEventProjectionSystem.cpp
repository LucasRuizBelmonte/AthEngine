#pragma region Includes
#include "GameplayEventProjectionSystem.h"
#pragma endregion

#pragma region Function Definitions
void GameplayEventProjectionSystem::Reset()
{
	m_hasProjectedStep = false;
	m_lastProjectedStep = 0u;
}

void GameplayEventProjectionSystem::Update(events::SceneEventBus &eventBus, uint64_t fixedStepCounter)
{
	eventBus.Clear<events::GameplayEvent>();

	if (m_hasProjectedStep && m_lastProjectedStep == fixedStepCounter)
		return;

	m_hasProjectedStep = true;
	m_lastProjectedStep = fixedStepCounter;

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
