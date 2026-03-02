#pragma once

#include "../IScene.h"
#include <memory>

class MultiScene final : public IScene
{
public:
	MultiScene(std::shared_ptr<IScene> a, std::shared_ptr<IScene> b);
	~MultiScene() override = default;

	void RequestLoad(AsyncLoader &loader) override;
	bool IsLoaded() const override;

	void OnAttach(GLFWwindow &window) override;
	void OnDetach(GLFWwindow &window) override;

	void Update(float dt, float now) override;
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

private:
	std::shared_ptr<IScene> m_a;
	std::shared_ptr<IScene> m_b;
};