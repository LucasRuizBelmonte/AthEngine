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
	void Update(Registry &registry);
	#pragma endregion
private:
	void EnsureEntityCapacity(uint32_t entityId);

	std::vector<Entity> m_entities;
	std::vector<uint32_t> m_visitStamp;
	std::vector<uint32_t> m_doneStamp;
	uint32_t m_currentStamp = 0;
};
#pragma endregion
