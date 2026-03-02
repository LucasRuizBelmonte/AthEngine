#pragma once

#include "../IScene.h"

class LoadingScene final : public IScene
{
public:
	LoadingScene() = default;
	~LoadingScene() override = default;

	const char *GetName() const override;

	void RequestLoad(AsyncLoader &loader) override;
	bool IsLoaded() const override;

	void OnAttach(GLFWwindow &window) override;
	void OnDetach(GLFWwindow &window) override;

	void Update(float dt, float now) override;
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

private:
	bool m_loaded = true;
};