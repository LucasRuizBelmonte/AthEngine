/**
 * @file UISpriteAssetSyncSystem.h
 * @brief Declarations for UISpriteAssetSyncSystem.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Registry.h"

#include <vector>
#pragma endregion

#pragma region Declarations
class TextureManager;
class ShaderManager;

class UISpriteAssetSyncSystem
{
public:
	void Update(Registry &registry, TextureManager &textureManager, ShaderManager &shaderManager);

private:
	std::vector<Entity> m_entities;
};
#pragma endregion
