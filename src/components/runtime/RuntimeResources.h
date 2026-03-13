/**
 * @file RuntimeResources.h
 * @brief ECS resource data used by runtime gameplay systems.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Entity.h"

#include <cstdint>
#pragma endregion

#pragma region Declarations
struct RuntimeSystemSwitches
{
	bool clearColor = true;
	bool cameraController = true;
	bool spin = false;
	bool render3D = true;
	bool render2D = true;
	bool spriteAnimation = true;
	bool triggerZoneConsole = true;
	bool uiRender = true;
};

struct RuntimeSceneFlags
{
	bool is2DScene = false;
	bool editorInputEnabled = false;
};

struct RuntimeSimulationClock
{
	float fixedSimulationNow = 0.0f;
	uint64_t fixedStepCounter = 0u;
};

struct RuntimePrimaryCamera
{
	Entity entity = kInvalidEntity;
};

struct GameplayEventProjectionState
{
	bool hasProjectedStep = false;
	uint64_t lastProjectedStep = 0u;
};
#pragma endregion
