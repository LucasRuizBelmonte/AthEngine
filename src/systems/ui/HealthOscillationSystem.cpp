#pragma region Includes
#include "HealthOscillationSystem.h"

#include "../../components/ui/Health.h"
#include "../../components/ui/HealthOscillator.h"

#include <algorithm>
#include <cmath>
#pragma endregion

#pragma region Function Definitions
void HealthOscillationSystem::Update(Registry &registry, float dt)
{
	registry.ViewEntities<Health, HealthOscillator>(m_entities);

	for (Entity entity : m_entities)
	{
		Health &health = registry.Get<Health>(entity);
		HealthOscillator &oscillator = registry.Get<HealthOscillator>(entity);

		oscillator.elapsed += dt;

		const float maxHealth = std::max(0.0f, health.max);
		const float oscillation = oscillator.center01 +
								  oscillator.amplitude01 * std::sin(oscillator.phase + oscillator.elapsed * oscillator.speed);
		const float normalized = std::clamp(oscillation, 0.0f, 1.0f);
		health.current = normalized * maxHealth;
	}
}
#pragma endregion
