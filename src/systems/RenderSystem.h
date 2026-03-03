/**
 * @file RenderSystem.h
 * @brief Declarations for RenderSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
class Renderer;

class RenderSystem
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
                int framebufferHeight) const;
	#pragma endregion
};
#pragma endregion
