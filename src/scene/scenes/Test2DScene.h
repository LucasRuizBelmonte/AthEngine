#pragma once

#include "../IScene.h"
#include "../AsyncLoader.h"
#include "../IEditorScene.h"

#include "../../ecs/Registry.h"
#include "../../systems/Render2DSystem.h"

#include "../../components/Camera.h"
#include "../../components/Transform.h"
#include "../../components/Sprite.h"
#include "../../components/Mesh.h"

#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"

#include "../../rendering/Texture.h"
#include "../../audio/AudioEngine.h"

class Test2DScene final : public IScene, public IEditorScene
{
public:
	Test2DScene(ShaderManager &shaderManager, TextureManager &textureManager);
	~Test2DScene() override = default;

	void RequestLoad(AsyncLoader &loader) override;
	bool IsLoaded() const override;

	void OnAttach(GLFWwindow &window) override;
	void OnDetach(GLFWwindow &window) override;

	void Update(float dt, float now) override;
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	const char *GetName() const override;

	Registry &GetEditorRegistry() override;
	void GetEditorSystems(std::vector<EditorSystemToggle> &out) override;

private:
	struct ImageData
	{
		int w = 0;
		int h = 0;
		std::vector<uint8_t> rgba;
	};

	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;

	Registry m_registry;
	Render2DSystem m_render2DSystem;

	bool m_sysAudio = true;
	bool m_sysRender2D = true;

	Entity m_camera2D = kInvalidEntity;
	Entity m_sprite = kInvalidEntity;

	Mesh m_quadMesh;

	GLFWwindow *m_window = nullptr;

	AudioEngine m_audio;

	bool m_loaded = false;

	std::string m_vsPath;
	std::string m_fsPath;
	std::string m_texPath;
};