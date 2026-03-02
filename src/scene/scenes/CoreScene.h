#pragma once

#include "../IScene.h"
#include <cstddef>
#include "../../ecs/Entity.h"

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

	void RenderSceneAdder();
	void RenderSceneList();
	void RenderEntityHierarchy();
	void RenderSystems();
	void RenderInspector();

private:
	SceneManager &m_scenes;
	bool m_loaded = true;

	size_t m_selectedScene = 0;
	Entity m_selectedEntity = kInvalidEntity;
};