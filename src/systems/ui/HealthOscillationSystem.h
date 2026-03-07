/**
 * @file HealthOscillationSystem.h
 * @brief Declarations for HealthOscillationSystem.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Registry.h"

#include <vector>
#pragma endregion

#pragma region Declarations
class HealthOscillationSystem
{
public:
	void Update(Registry &registry, float dt);

private:
	std::vector<Entity> m_entities;
};
#pragma endregion
