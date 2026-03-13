/**
 * @file Physics2DSystem.h
 * @brief Declarations for Physics2DSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#include "../events/SceneEventBus.h"
#include "Physics2DWorldState.h"

#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
class Physics2DSystem
{
public:
#pragma region Public Interface
	using StepStats = Physics2DStepStats;

	void ResetWorldState(Registry &registry) const;
	void SetGravity(Registry &registry, const glm::vec2 &gravity) const;
	glm::vec2 GetGravity(const Registry &registry) const;
	const StepStats &GetLastStepStats(const Registry &registry) const;
	void FixedUpdate(Registry &registry, float fixedDt, events::SceneEventBus &eventBus) const;
#pragma endregion
private:
	void EmitEvents(events::SceneEventBus &eventBus, Physics2DWorldState &world) const;
};
#pragma endregion
