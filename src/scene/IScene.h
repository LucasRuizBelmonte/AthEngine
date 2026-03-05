/**
 * @file IScene.h
 * @brief Declarations for IScene.
 */

#pragma once

#pragma region Includes
#include "../platform/GL.h"
#include "../input/InputActions.h"
#pragma endregion

#pragma region Declarations
class Renderer;
class AsyncLoader;

class IScene
{
public:
	#pragma region Public Interface
    /**
     * @brief Executes ~IScene.
     */
    virtual ~IScene() = default;

    /**
     * @brief Executes Get Name.
     */
    virtual const char* GetName() const = 0;

    /**
     * @brief Executes Request Load.
     */
    virtual void RequestLoad(AsyncLoader &loader) = 0;
    /**
     * @brief Executes Is Loaded.
     */
    virtual bool IsLoaded() const = 0;

    /**
     * @brief Executes On Attach.
     */
    virtual void OnAttach(GLFWwindow &window) = 0;
    /**
     * @brief Executes On Detach.
     */
    virtual void OnDetach(GLFWwindow &window) = 0;

    /**
     * @brief Executes Update.
     */
    virtual void Update(float dt, float now, const InputState &input) = 0;
    /**
     * @brief Executes Fixed Update.
     */
    virtual void FixedUpdate(float fixedDt) = 0;
    /**
     * @brief Executes Render3 D.
     */
    virtual void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) = 0;
    /**
     * @brief Executes Render2 D.
     */
    virtual void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) = 0;
	#pragma endregion
};
#pragma endregion
