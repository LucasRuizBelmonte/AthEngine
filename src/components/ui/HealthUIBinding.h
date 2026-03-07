/**
 * @file HealthUIBinding.h
 * @brief Maps a health entity to UI targets.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Entity.h"

#include <string>
#pragma endregion

#pragma region Declarations
struct HealthUIBinding
{
	Entity fillEntity = kInvalidEntity;
	Entity labelEntity = kInvalidEntity;
	std::string labelPrefix = "HP ";
};
#pragma endregion
