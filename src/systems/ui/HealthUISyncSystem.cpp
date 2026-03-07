#pragma region Includes
#include "HealthUISyncSystem.h"

#include "../../components/ui/Health.h"
#include "../../components/ui/HealthUIBinding.h"
#include "../../components/ui/UIFill.h"
#include "../../components/ui/UIText.h"

#include <algorithm>
#include <cmath>
#include <string>
#pragma endregion

#pragma region Function Definitions
void HealthUISyncSystem::Update(Registry &registry)
{
	registry.ViewEntities<Health, HealthUIBinding>(m_entities);

	for (Entity entity : m_entities)
	{
		const Health &health = registry.Get<Health>(entity);
		const HealthUIBinding &binding = registry.Get<HealthUIBinding>(entity);

		const float maxHealth = std::max(0.0f, health.max);
		float normalized = 0.0f;
		if (maxHealth > 0.0f)
			normalized = std::clamp(health.current / maxHealth, 0.0f, 1.0f);

		if (binding.fillEntity != kInvalidEntity &&
			registry.IsAlive(binding.fillEntity) &&
			registry.Has<UIFill>(binding.fillEntity))
		{
			registry.Get<UIFill>(binding.fillEntity).value01 = normalized;
		}

		if (binding.labelEntity != kInvalidEntity &&
			registry.IsAlive(binding.labelEntity) &&
			registry.Has<UIText>(binding.labelEntity))
		{
			const int percentage = static_cast<int>(std::round(normalized * 100.0f));
			registry.Get<UIText>(binding.labelEntity).text = binding.labelPrefix + std::to_string(percentage) + "%";
		}
	}
}
#pragma endregion
