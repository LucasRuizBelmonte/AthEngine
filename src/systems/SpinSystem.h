/**
 * @file SpinSystem.h
 * @brief Declarations for SpinSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
class SpinSystem
{
public:
	#pragma region Public Interface
	/**
	 * @brief Executes Update.
	 */
	void Update(Registry &registry, float timeSec) const;
	#pragma endregion
};
#pragma endregion
