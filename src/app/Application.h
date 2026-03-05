/**
 * @file Application.h
 * @brief Declarations for Application.
 */

#pragma once

#include "../platform/GlfwContext.h"
#include "../platform/Window.h"
#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"
#include "../rendering/Renderer.h"
#include "../scene/SceneManager.h"

#include <memory>

/**
 * @brief Main application class that manages the game engine lifecycle.
 *
 * Handles window management, rendering, scene updates, and input processing.
 * Coordinates all subsystems (GLFW, OpenGL, rendering, scene management, ImGui).
 */
class Application
{
public:
#pragma region Lifecycle

	/**
	 * @brief Constructs the application, initializing core subsystems.
	 *
	 * Initializes GLFW via `GlfwContext` and allocates managers for shaders,
	 * textures, rendering, and scene handling. Should be called once at
	 * program start.
	 */
	Application();

	/**
	 * @brief Destructs the application and releases resources.
	 *
	 * Automatically destroys managed objects and shuts down GLFW when the
	 * last context is gone.
	 */
	~Application() noexcept;

	/**
	 * @brief Runs the main application loop.
	 *
	 * Processes input, updates the active scene, and renders frames until the
	 * window should close. This call blocks until exit.
	 */
	void Run();

#pragma endregion

#pragma region Input Handling

	/**
	 * @brief Toggles mouse capture mode.
	 *
	 * Switches between disabled (game) mode and normal (UI) mode.
	 */
	void ToggleMouseCapture();

#pragma endregion

#pragma region ImGui Integration

	/**
	 * @brief Executes Init Im Gui.
	 */
	void InitImGui();
	/**
	 * @brief Executes Shutdown Im Gui.
	 */
	void ShutdownImGui();
	/**
	 * @brief Executes Begin Im Gui Frame.
	 */
	void BeginImGuiFrame();
	/**
	 * @brief Executes End Im Gui Frame.
	 */
	void EndImGuiFrame();

#pragma endregion

private:
	/**
	 * @brief Executes Ensure Scene Render Target.
	 */
	void EnsureSceneRenderTarget(int width, int height);
	/**
	 * @brief Executes Destroy Scene Render Target.
	 */
	void DestroySceneRenderTarget();

#pragma region Input Processing

	/**
	 * @brief Process input related to scene management.
	 *
	 * Handles key latches for switching scenes or emitting debug commands.
	 * Called every frame from `Run()`.
	 */
	void HandleSceneInput();

#pragma endregion

#pragma region Core Subsystems

	GlfwContext m_Glfw;

	std::unique_ptr<Window> m_Window;
	ShaderManager m_ShaderManager;
	TextureManager m_TextureManager;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<SceneManager> m_Scenes;

#pragma endregion

#pragma region Frame Timing

	float m_LastTime = 0.0f;

#pragma endregion

	unsigned int m_SceneFbo = 0;
	unsigned int m_SceneColorTexture = 0;
	unsigned int m_SceneDepthRbo = 0;
	int m_SceneTargetWidth = 0;
	int m_SceneTargetHeight = 0;

#pragma region Input State

	double m_LastMouseX = 0.0;
	double m_LastMouseY = 0.0;
	bool m_MouseCaptured = false;
	bool m_RightMouseCaptureOwned = false;
	bool m_imguiInitialized = false;

#pragma endregion
};
