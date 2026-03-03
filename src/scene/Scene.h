/**
 * @file Scene.h
 * @brief Declarations for Scene.
 */

#pragma once

#pragma region Includes
#include "../platform/GL.h"

#include "../ecs/Registry.h"
#include "../systems/ClearColorSystem.h"
#include "../systems/SpinSystem.h"
#include "../systems/RenderSystem.h"
#include "../systems/Render2DSystem.h"
#include "../systems/CameraControllerSystem.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Transform.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Spin.h"
#include "../components/Sprite.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include "../input/WindowContext.h"
#pragma endregion

#pragma region Declarations
class Renderer;

class Scene
{
public:
	#pragma region Public Interface
	/**
	 * @brief Constructs a new Scene instance.
	 */
	Scene(ShaderManager &shaderManager, TextureManager &textureManager, GLFWwindow &window);
	/**
	 * @brief Destroys this Scene instance.
	 */
	~Scene();

	/**
	 * @brief Executes Update.
	 */
	void Update(float dt, float now);
	/**
	 * @brief Executes Render3 D.
	 */
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
	/**
	 * @brief Executes Render2 D.
	 */
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);

	#pragma endregion
private:
	#pragma region Private Implementation
	Registry m_Registry;

	ClearColorSystem m_ClearColorSystem;
	SpinSystem m_SpinSystem;
	RenderSystem m_RenderSystem;
	Render2DSystem m_Render2DSystem;
	CameraControllerSystem m_CameraControllerSystem;

	Entity m_Camera3D = kInvalidEntity;
	Entity m_Camera2D = kInvalidEntity;

	Entity m_Triangle = kInvalidEntity;
	Entity m_Sprite = kInvalidEntity;

	Mesh m_QuadMesh;

	GLFWwindow *m_Window = nullptr;
	WindowContext m_WindowCtx;
	#pragma endregion
};
#pragma endregion
