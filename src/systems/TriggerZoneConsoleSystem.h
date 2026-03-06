/**
 * @file TriggerZoneConsoleSystem.h
 * @brief Declarations for TriggerZoneConsoleSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#include "../events/SceneEventBus.h"
#pragma endregion

#pragma region Declarations
class TriggerZoneConsoleSystem
{
public:
	#pragma region Public Interface
	/**
	 * @brief Emits console logs when trigger-zone events are raised.
	 */
	void Update(const Registry &registry, const events::SceneEventBus &eventBus) const;
	#pragma endregion
};
#pragma endregion
