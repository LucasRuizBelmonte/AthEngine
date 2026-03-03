/**
 * @file MultiScene.h
 * @brief Declarations for MultiScene.
 */

#pragma once

#pragma region Includes
#include "../IScene.h"
#include <memory>
#pragma endregion

#pragma region Declarations
class MultiScene final : public IScene
{
public:
	#pragma region Public Interface
	/**
	 * @brief Constructs a new MultiScene instance.
	 */
	MultiScene(std::shared_ptr<IScene> a, std::shared_ptr<IScene> b);
	~MultiScene() override = default;

	/**
	 * @brief Executes Request Load.
	 */
	void RequestLoad(AsyncLoader &loader) override;
	/**
	 * @brief Executes Is Loaded.
	 */
	bool IsLoaded() const override;

	/**
	 * @brief Executes On Attach.
	 */
	void OnAttach(GLFWwindow &window) override;
	/**
	 * @brief Executes On Detach.
	 */
	void OnDetach(GLFWwindow &window) override;

	/**
	 * @brief Executes Update.
	 */
	void Update(float dt, float now) override;
	/**
	 * @brief Executes Render3 D.
	 */
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	/**
	 * @brief Executes Render2 D.
	 */
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	/**
	 * @brief Executes Get Name.
	 */
	const char *GetName() const override;

	#pragma endregion
private:
	#pragma region Private Implementation
	std::shared_ptr<IScene> m_a;
	std::shared_ptr<IScene> m_b;
	#pragma endregion
};
#pragma endregion
