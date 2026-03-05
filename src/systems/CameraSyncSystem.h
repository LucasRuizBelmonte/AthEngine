/**
 * @file CameraSyncSystem.h
 * @brief Declarations for CameraSyncSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
class CameraSyncSystem
{
public:
	#pragma region Public Interface
	/**
	 * @brief Syncs all camera components from their transform components.
	 */
	void SyncAllFromTransform(Registry &registry, bool force2DMode);

	/**
	 * @brief Syncs one camera transform from camera state.
	 */
	void SyncTransformFromCamera(Registry &registry, Entity cameraEntity, bool force2DMode) const;
	#pragma endregion

private:
	std::vector<Entity> m_cameraEntities;
};
#pragma endregion
