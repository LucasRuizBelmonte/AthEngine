/**
 * @file SpriteAnimationSystem.h
 * @brief Declarations for SpriteAnimationSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"

#include <string>
#include <vector>
#pragma endregion

#pragma region Declarations
class Animation2DLibrary;
class TextureManager;

class SpriteAnimationSystem
{
public:
	struct FrameEvent
	{
		Entity entity = kInvalidEntity;
		std::string clipId;
		int frame = 0;
	};

	#pragma region Public Interface
	void Update(Registry &registry, const Animation2DLibrary &library, TextureManager &textureManager, float dt);
	const std::vector<FrameEvent> &GetFrameEvents() const;
	void ClearFrameEvents();
	#pragma endregion
private:
	std::vector<Entity> m_items;
	std::vector<FrameEvent> m_frameEvents;
};
#pragma endregion
