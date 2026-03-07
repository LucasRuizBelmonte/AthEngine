/**
 * @file HealthUISyncSystem.h
 * @brief Declarations for HealthUISyncSystem.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Registry.h"

#include <vector>
#pragma endregion

#pragma region Declarations
class HealthUISyncSystem
{
public:
	void Update(Registry &registry);

private:
	std::vector<Entity> m_entities;
};
#pragma endregion
