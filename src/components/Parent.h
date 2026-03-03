/**
 * @file Parent.h
 * @brief Declarations for Parent.
 */

#pragma once

#pragma region Includes
#include "../ecs/Entity.h"
#pragma endregion

#pragma region Declarations
struct Parent
{
	Entity parent = kInvalidEntity;
};
#pragma endregion
