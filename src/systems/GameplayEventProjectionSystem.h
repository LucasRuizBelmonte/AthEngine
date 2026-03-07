/**
 * @file GameplayEventProjectionSystem.h
 * @brief Declarations for GameplayEventProjectionSystem.
 */

#pragma once

#pragma region Includes
#include "../events/SceneEventBus.h"

#include <cstdint>
#pragma endregion

#pragma region Declarations
class GameplayEventProjectionSystem
{
public:
	void Reset();
	void Update(events::SceneEventBus &eventBus, uint64_t fixedStepCounter);

private:
	bool m_hasProjectedStep = false;
	uint64_t m_lastProjectedStep = 0u;
};
#pragma endregion
