/**
 * @file CameraControllerSystem.h
 * @brief Declarations for CameraControllerSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
struct GLFWwindow;

class CameraControllerSystem
{
public:
	#pragma region Public Interface
	/**
	 * @brief Executes Update.
	 */
	void Update(Registry& registry, GLFWwindow& window, Entity cameraEntity, float dt, bool is2DMode) const;
	#pragma endregion
};
#pragma endregion
