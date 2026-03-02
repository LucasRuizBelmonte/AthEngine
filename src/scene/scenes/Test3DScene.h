#pragma once

#include "../IScene.h"
#include "../AsyncLoader.h"

#include "../../ecs/Registry.h"
#include "../../systems/ClearColorSystem.h"
#include "../../systems/SpinSystem.h"
#include "../../systems/RenderSystem.h"
#include "../../systems/CameraControllerSystem.h"

#include "../../components/Camera.h"
#include "../../components/CameraController.h"
#include "../../components/Transform.h"
#include "../../components/Mesh.h"
#include "../../components/Material.h"
#include "../../components/Spin.h"

#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"

#include "../../input/WindowContext.h"

class Test3DScene final : public IScene
{
public:
	Test3DScene(ShaderManager &shaderManager, TextureManager &textureManager);
	~Test3DScene() override = default;

	void RequestLoad(AsyncLoader &loader) override;
	bool IsLoaded() const override;

	void OnAttach(GLFWwindow &window) override;
	void OnDetach(GLFWwindow &window) override;

	void Update(float dt, float now) override;
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

private:
	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;

	Registry m_registry;

	ClearColorSystem m_clearColorSystem;
	SpinSystem m_spinSystem;
	RenderSystem m_renderSystem;
	CameraControllerSystem m_cameraControllerSystem;

	Entity m_camera3D = kInvalidEntity;
	Entity m_triangle = kInvalidEntity;

	GLFWwindow *m_window = nullptr;
	WindowContext m_windowCtx;

	bool m_loaded = false;

	std::string m_vsPath;
	std::string m_fsPath;
};