/**
 * @file TransformSystem.h
 * @brief Declarations for TransformSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
class TransformSystem
{
public:
	#pragma region Public Interface
	/**
	 * @brief Computes world transforms from local transforms and hierarchy.
	 */
	void Update(Registry &registry) const;
	#pragma endregion
};
#pragma endregion
