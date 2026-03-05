/**
 * @file RenderSystem.h
 * @brief Declarations for RenderSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#include "../rendering/Renderer.h"
#pragma endregion

#pragma region Declarations
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
                int framebufferHeight);
	#pragma endregion
private:
	std::vector<Entity> m_lightEntities;
	std::vector<Renderer::LightData> m_lights;
	std::vector<Entity> m_renderEntities;
};
#pragma endregion
