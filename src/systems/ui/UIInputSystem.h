/**
 * @file UIInputSystem.h
 * @brief Declarations for UIInputSystem.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Registry.h"
#include "../../input/InputActions.h"
#pragma endregion

#pragma region Declarations
class UIInputSystem
{
public:
	void Update(Registry &registry, const InputState &input, float dt) const;
};
#pragma endregion
