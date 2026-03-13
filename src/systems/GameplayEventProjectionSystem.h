/**
 * @file GameplayEventProjectionSystem.h
 * @brief Declarations for GameplayEventProjectionSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#include "../events/SceneEventBus.h"
#pragma endregion

#pragma region Declarations
class GameplayEventProjectionSystem
{
public:
	void Reset(Registry &registry) const;
	void Update(Registry &registry, events::SceneEventBus &eventBus) const;
};
#pragma endregion
