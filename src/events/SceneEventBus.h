/**
 * @file SceneEventBus.h
 * @brief Declarations for a per-scene event bus.
 */

#pragma once

#pragma region Includes
#include "EventQueue.h"
#include "GameplayEvents.h"
#include "PhysicsEvents.h"
#pragma endregion

#pragma region Declarations
namespace events
{
	using SceneEventBus = EventQueue<
		PhysicsCollisionEvent2D,
		PhysicsTriggerEvent2D,
		GameplayEvent>;
}
#pragma endregion
