#pragma once
#include "../IScene.h"
#include "../../editor/EditorUI.h"

class SceneManager;

class CoreScene final : public IScene
{
public:
	explicit CoreScene(SceneManager &scenes);
	~CoreScene() override = default;

	const char *GetName() const override;

	void RequestLoad(AsyncLoader &loader) override;
	bool IsLoaded() const override;

	void OnAttach(GLFWwindow &window) override;
	void OnDetach(GLFWwindow &window) override;

	void Update(float dt, float now) override;
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

private:
	SceneManager &m_scenes;
	bool m_loaded = true;
	EditorUIState m_ui;
};