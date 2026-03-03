/**
 * @file Render2DSystem.h
 * @brief Declarations for Render2DSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
class Renderer;

class Render2DSystem
{
public:
	#pragma region Public Interface
	/**
	 * @brief Executes Render.
	 */
	void Render(Registry &registry,
				Renderer &renderer,
				Entity cameraEntity,
				int framebufferWidth,
				int framebufferHeight,
				const struct Mesh &quadMesh) const;
	#pragma endregion
};
#pragma endregion
