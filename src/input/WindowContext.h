/**
 * @file WindowContext.h
 * @brief Declarations for WindowContext.
 */

#pragma once

#pragma region Includes
#include "../ecs/Entity.h"
#pragma endregion

#pragma region Declarations
class Registry;

struct WindowContext
{
	Registry *registry = nullptr;
	Entity cameraEntity = kInvalidEntity;
};
#pragma endregion
