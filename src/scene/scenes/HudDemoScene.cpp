#pragma region Includes
#include "../../platform/GL.h"
#include "HudDemoScene.h"

#include "../../components/Parent.h"
#include "../../utils/AssetPath.h"

#include <string>
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
HudDemoScene::HudDemoScene(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager),
	  m_textureManager(textureManager)
{
}

const char *HudDemoScene::GetName() const
{
	return "HUD Demo";
}

void HudDemoScene::RequestLoad(AsyncLoader &)
{
	BuildHud();
	m_loaded = true;
}

bool HudDemoScene::IsLoaded() const
{
	return m_loaded;
}

void HudDemoScene::OnAttach(GLFWwindow &)
{
}

void HudDemoScene::OnDetach(GLFWwindow &)
{
}

void HudDemoScene::Update(float dt, float, const InputState &input)
{
	if (!m_loaded)
		return;

	m_uiInputSystem.Update(m_registry, input, dt);
	m_healthOscillationSystem.Update(m_registry, dt);
	m_healthUISyncSystem.Update(m_registry);
	m_uiSpriteAssetSyncSystem.Update(m_registry, m_textureManager, m_shaderManager);
}

void HudDemoScene::FixedUpdate(float)
{
}

void HudDemoScene::Render3D(Renderer &, int, int)
{
}

void HudDemoScene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded)
		return;

	glDisable(GL_DEPTH_TEST);
	m_uiLayoutSystem.Update(m_registry, framebufferWidth, framebufferHeight);
	m_uiTransformSystem.Update(m_registry, framebufferWidth, framebufferHeight);
	m_uiRenderSystem.Render(m_registry, renderer, m_shaderManager, m_textureManager, framebufferWidth, framebufferHeight);
}

void HudDemoScene::BuildHud()
{
	ClearRegistry(m_registry);

	const auto iconTexture = m_textureManager.Load("hud_demo_icon", AssetPath::ResolveRuntimePath("res/textures/sprite_4.png"), true);

	const Entity canvas = m_registry.Create();
	{
		UITransform t;
		t.anchorMin = glm::vec2(0.0f, 0.0f);
		t.anchorMax = glm::vec2(1.0f, 1.0f);
		t.pivot = glm::vec2(0.5f, 0.5f);
		t.sizeDelta = glm::vec2(0.0f, 0.0f);
		t.anchoredPosition = glm::vec2(0.0f, 0.0f);
		m_registry.Emplace<UITransform>(canvas, t);
	}

	const Entity topLeftRow = m_registry.Create();
	m_registry.Emplace<Parent>(topLeftRow, Parent{canvas});
	{
		UITransform t;
		t.anchorMin = glm::vec2(0.0f, 0.0f);
		t.anchorMax = glm::vec2(0.0f, 0.0f);
		t.pivot = glm::vec2(0.0f, 0.0f);
		t.anchoredPosition = glm::vec2(24.0f, 24.0f);
		t.sizeDelta = glm::vec2(430.0f, 96.0f);
		m_registry.Emplace<UITransform>(topLeftRow, t);

		UIHorizontalGroup group;
		group.padding = UIPadding{8.0f, 8.0f, 8.0f, 8.0f};
		group.spacing = 12.0f;
		group.childAlignment = UIChildAlignment::Center;
		group.expandHeight = true;
		m_registry.Emplace<UIHorizontalGroup>(topLeftRow, group);
	}

	const Entity icon = m_registry.Create();
	m_registry.Emplace<Parent>(icon, Parent{topLeftRow});
	m_registry.Emplace<UITransform>(icon, UITransform{});
	m_registry.Emplace<UILayoutElement>(icon, UILayoutElement{
		glm::vec2(48.0f, 48.0f),
		glm::vec2(48.0f, 48.0f),
		glm::vec2(0.0f, 0.0f)});
	{
		UISprite sprite;
		sprite.texture = iconTexture;
		sprite.texturePath = "res/textures/sprite_4.png";
		sprite.materialPath = "res/shaders/unlit2D.fs";
		sprite.tint = glm::vec4(1.0f, 0.95f, 0.95f, 1.0f);
		sprite.preserveAspect = true;
		sprite.layer = 10;
		sprite.orderInLayer = 0;
		m_registry.Emplace<UISprite>(icon, sprite);
	}

	const Entity spacer = m_registry.Create();
	m_registry.Emplace<Parent>(spacer, Parent{topLeftRow});
	m_registry.Emplace<UITransform>(spacer, UITransform{});
	m_registry.Emplace<UISpacer>(spacer, UISpacer{
		glm::vec2(14.0f, 1.0f),
		glm::vec2(0.0f, 0.0f)});

	const Entity rightColumn = m_registry.Create();
	m_registry.Emplace<Parent>(rightColumn, Parent{topLeftRow});
	m_registry.Emplace<UITransform>(rightColumn, UITransform{});
	m_registry.Emplace<UILayoutElement>(rightColumn, UILayoutElement{
		glm::vec2(300.0f, 64.0f),
		glm::vec2(300.0f, 64.0f),
		glm::vec2(1.0f, 0.0f)});
	{
		UIVerticalGroup group;
		group.spacing = 7.0f;
		group.childAlignment = UIChildAlignment::Start;
		group.expandWidth = true;
		m_registry.Emplace<UIVerticalGroup>(rightColumn, group);
	}

	const Entity healthBarMask = m_registry.Create();
	m_registry.Emplace<Parent>(healthBarMask, Parent{rightColumn});
	m_registry.Emplace<UITransform>(healthBarMask, UITransform{});
	m_registry.Emplace<UILayoutElement>(healthBarMask, UILayoutElement{
		glm::vec2(280.0f, 24.0f),
		glm::vec2(280.0f, 24.0f),
		glm::vec2(1.0f, 0.0f)});
	m_registry.Emplace<UIMask>(healthBarMask, UIMask{true});

	const Entity healthBarBackground = m_registry.Create();
	m_registry.Emplace<Parent>(healthBarBackground, Parent{healthBarMask});
	{
		UITransform t;
		t.anchorMin = glm::vec2(0.0f, 0.0f);
		t.anchorMax = glm::vec2(1.0f, 1.0f);
		t.sizeDelta = glm::vec2(0.0f, 0.0f);
		m_registry.Emplace<UITransform>(healthBarBackground, t);
	}
	m_registry.Emplace<UISprite>(healthBarBackground, UISprite{
		{},
		{},
		"",
		"res/shaders/unlit2D.fs",
		glm::vec4(0.25f, 0.04f, 0.04f, 0.95f),
		glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
		10,
		0,
		false});

	const Entity healthFillEntity = m_registry.Create();
	m_registry.Emplace<Parent>(healthFillEntity, Parent{healthBarMask});
	{
		UITransform t;
		t.anchorMin = glm::vec2(0.0f, 0.0f);
		t.anchorMax = glm::vec2(1.0f, 1.0f);
		t.sizeDelta = glm::vec2(0.0f, 0.0f);
		m_registry.Emplace<UITransform>(healthFillEntity, t);
	}
	m_registry.Emplace<UISprite>(healthFillEntity, UISprite{
		{},
		{},
		"",
		"res/shaders/unlit2D.fs",
		glm::vec4(0.19f, 0.85f, 0.24f, 1.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
		10,
		1,
		false});
	m_registry.Emplace<UIFill>(healthFillEntity, UIFill{1.0f, UIFillDirection::LeftToRight});

	const Entity healthLabelEntity = m_registry.Create();
	m_registry.Emplace<Parent>(healthLabelEntity, Parent{rightColumn});
	m_registry.Emplace<UITransform>(healthLabelEntity, UITransform{});
	m_registry.Emplace<UILayoutElement>(healthLabelEntity, UILayoutElement{
		glm::vec2(280.0f, 24.0f),
		glm::vec2(280.0f, 24.0f),
		glm::vec2(1.0f, 0.0f)});
	m_registry.Emplace<UIText>(healthLabelEntity, UIText{
		"HP 100%",
		glm::vec4(0.97f, 0.97f, 0.97f, 1.0f),
		"builtin_mono_5x7",
		18.0f,
		UITextAlignment::Left,
		false,
		true,
		glm::vec4(0.05f, 0.05f, 0.05f, 1.0f),
		1.0f,
		11,
		0});

	const Entity healthEntity = m_registry.Create();
	m_registry.Emplace<Health>(healthEntity, Health{100.0f, 100.0f});
	m_registry.Emplace<HealthOscillator>(healthEntity, HealthOscillator{});
	HealthUIBinding binding;
	binding.fillEntity = healthFillEntity;
	binding.labelEntity = healthLabelEntity;
	binding.labelPrefix = "HP ";
	m_registry.Emplace<HealthUIBinding>(healthEntity, binding);

	m_healthUISyncSystem.Update(m_registry);
}
#pragma endregion
