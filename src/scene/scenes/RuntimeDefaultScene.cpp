#pragma region Includes
#include "../../platform/GL.h"
#include "RuntimeDefaultScene.h"

#include "../../components/Tag.h"

#include <vector>
#include <utility>
#include <glm/gtc/matrix_transform.hpp>
#pragma endregion

#pragma region File Scope
namespace
{
	static void ClearRegistry(Registry &registry)
	{
		const std::vector<Entity> alive = registry.Alive();
		for (Entity entity : alive)
			registry.Destroy(entity);
	}
}
#pragma endregion

#pragma region Function Definitions
RuntimeDefaultScene::RuntimeDefaultScene(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager),
	  m_textureManager(textureManager)
{
	RegisterBuiltin2DAnimationClips();
}

const char *RuntimeDefaultScene::GetName() const
{
	return "RuntimeDefault";
}

void RuntimeDefaultScene::RequestLoad(AsyncLoader &)
{
	m_loaded = false;
	m_fixedSimulationNow = 0.0f;
	m_fixedStepCounter = 0u;
	m_gameplayEventProjectionSystem.Reset();
	m_eventBus.ClearAll();
	BuildBaseTemplate();
	RefreshRuntimeReferences();
	m_loaded = true;
}

bool RuntimeDefaultScene::IsLoaded() const
{
	return m_loaded;
}

void RuntimeDefaultScene::OnAttach(GLFWwindow &window)
{
	m_window = &window;
}

void RuntimeDefaultScene::OnDetach(GLFWwindow &window)
{
	if (&window == m_window)
		m_window = nullptr;
}

void RuntimeDefaultScene::Update(float dt, float now, const InputState &input)
{
	if (!m_loaded)
		return;

	m_gameplayEventProjectionSystem.Update(m_eventBus, m_fixedStepCounter);

	if (m_sysClearColor)
		m_clearColorSystem.Update(now);

	const Entity camera = ResolvePrimaryCamera();
	if (camera != kInvalidEntity && m_sysCameraController && m_window)
	{
		m_cameraControllerSystem.Update(m_registry, *m_window, camera, dt, false, input);
		m_cameraSyncSystem.SyncTransformFromCamera(m_registry, camera, false);
	}

	m_transformSystem.Update(m_registry);
	m_cameraSyncSystem.SyncAllFromTransform(m_registry, false);
	if (m_sysSpriteAnimation)
		m_spriteAnimationSystem.Update(m_registry, m_animation2DLibrary, m_textureManager, dt);
	m_uiSpriteAssetSyncSystem.Update(m_registry, m_textureManager, m_shaderManager);
	m_uiInputSystem.Update(m_registry, input, dt);
}

void RuntimeDefaultScene::FixedUpdate(float fixedDt)
{
	if (!m_loaded)
		return;

	m_fixedSimulationNow += fixedDt;
	if (m_sysSpin)
		m_spinSystem.Update(m_registry, m_fixedSimulationNow);

	m_physics2DSystem.FixedUpdate(m_registry, fixedDt, m_eventBus);
	if (m_sysTriggerZoneConsole)
		m_triggerZoneConsoleSystem.Update(m_registry, m_eventBus);
	++m_fixedStepCounter;

	m_transformSystem.Update(m_registry);
	m_cameraSyncSystem.SyncAllFromTransform(m_registry, false);
}

void RuntimeDefaultScene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded || !m_sysRender)
		return;

	const Entity camera = ResolvePrimaryCamera();
	if (camera == kInvalidEntity)
		return;

	glEnable(GL_DEPTH_TEST);
	m_renderSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight);
}

void RuntimeDefaultScene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded)
		return;

	if (m_sysRender2D)
	{
		const Entity camera = ResolvePrimaryCamera();
		if (camera != kInvalidEntity)
		{
			glDisable(GL_DEPTH_TEST);
			m_render2DSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight);
		}
	}

	if (m_sysUIRender)
	{
		glDisable(GL_DEPTH_TEST);
		m_uiLayoutSystem.Update(m_registry, framebufferWidth, framebufferHeight);
		m_uiTransformSystem.Update(m_registry, framebufferWidth, framebufferHeight);
		m_uiRenderSystem.Render(m_registry, renderer, m_shaderManager, m_textureManager, framebufferWidth, framebufferHeight);
	}
}

void RuntimeDefaultScene::BuildBaseTemplate()
{
	ClearRegistry(m_registry);

	const Entity cameraEntity = m_registry.Create();
	m_registry.Emplace<Tag>(cameraEntity, Tag{"Camera"});
	{
		Transform t;
		t.localPosition = glm::vec3(0.f, 0.f, 2.f);
		m_registry.Emplace<Transform>(cameraEntity, t);
	}
	m_registry.Emplace<Camera>(cameraEntity);

	const Entity lightEntity = m_registry.Create();
	m_registry.Emplace<Tag>(lightEntity, Tag{"DirectionalLight"});
	m_registry.Emplace<Transform>(lightEntity);
	{
		LightEmitter light;
		light.type = LightType::Directional;
		m_registry.Emplace<LightEmitter>(lightEntity, light);
	}
}

void RuntimeDefaultScene::RegisterBuiltin2DAnimationClips()
{
	SpriteAnimationClip sampleClip;
	sampleClip.fps = 8.f;
	sampleClip.loop = true;
	sampleClip.framesUV = {
		glm::vec4{0.0f, 0.0f, 0.5f, 0.5f},
		glm::vec4{0.5f, 0.0f, 1.0f, 0.5f},
		glm::vec4{0.0f, 0.5f, 0.5f, 1.0f},
		glm::vec4{0.5f, 0.5f, 1.0f, 1.0f}};

	m_animation2DLibrary.RegisterClip("sample.sprite_sheet.2x2", std::move(sampleClip));
}

void RuntimeDefaultScene::RefreshRuntimeReferences()
{
	m_camera = kInvalidEntity;
	std::vector<Entity> cameras;
	m_registry.ViewEntities<Camera>(cameras);
	if (!cameras.empty())
		m_camera = cameras.front();
}

Entity RuntimeDefaultScene::ResolvePrimaryCamera()
{
	if (m_camera != kInvalidEntity &&
	    m_registry.IsAlive(m_camera) &&
	    m_registry.Has<Camera>(m_camera) &&
	    m_registry.IsComponentEnabled<Camera>(m_camera))
	{
		return m_camera;
	}

	RefreshRuntimeReferences();
	return m_camera;
}
#pragma endregion
