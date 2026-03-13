/**
 * @file PrimaryCameraSystem.h
 * @brief ECS system that resolves and caches the active camera entity.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
class PrimaryCameraSystem
{
public:
	void Reset(Registry &registry) const;
	Entity Resolve(Registry &registry) const;
};
#pragma endregion
