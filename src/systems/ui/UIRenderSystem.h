/**
 * @file UIRenderSystem.h
 * @brief Declarations for UIRenderSystem.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Registry.h"
#include "../../rendering/Renderer.h"
#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"
#include "../../components/ui/UIComponents.h"
#include "../../ui/BitmapFont.h"

#include <optional>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region Declarations
class UIRenderSystem
{
public:
	void Render(Registry &registry,
	            Renderer &renderer,
	            ShaderManager &shaderManager,
	            TextureManager &textureManager,
	            int framebufferWidth,
	            int framebufferHeight);

private:
	struct RenderItem
	{
		Entity entity = kInvalidEntity;
		bool isText = false;
		int layer = 0;
		int orderInLayer = 0;
		int hierarchy = 0;
	};

	void EnsureResources(ShaderManager &shaderManager, TextureManager &textureManager);
	void EnsureBuiltinFontAtlas(TextureManager &textureManager);
	void BuildRenderList(Registry &registry);

	void RenderSpriteEntity(Registry &registry,
	                        Renderer &renderer,
	                        TextureManager &textureManager,
	                        uint32_t quadMeshId,
	                        Entity entity,
	                        int framebufferHeight);

	void RenderTextEntity(Registry &registry,
	                      Renderer &renderer,
	                      uint32_t quadMeshId,
	                      Entity entity,
	                      int framebufferHeight);

	std::optional<UIRect> CollectMaskClipRect(Registry &registry, Entity entity, const UIRect &screenRect) const;
	bool ApplyScissor(const std::optional<UIRect> &clipRect, int framebufferHeight) const;

	glm::mat4 BuildModelFromAdjustedRect(const UITransform &transform, const UIRect &adjustedRect) const;
	void SubmitQuad(Renderer &renderer,
	                uint32_t quadMeshId,
	                const glm::mat4 &model,
	                const glm::vec4 &tint,
	                const glm::vec4 &uv,
	                const ResourceHandle<Texture> &texture,
	                const ResourceHandle<Shader> &shader,
	                const std::string &materialPath) const;

	void DrawTextLineGlyphs(Renderer &renderer,
	                        uint32_t quadMeshId,
	                        const UITransform &transform,
	                        const UIText &text,
	                        const ui::BitmapFont &font,
	                        const std::string &line,
	                        float originX,
	                        float originY) const;
	bool TryGetBuiltinGlyphUv(char c, glm::vec4 &outUv) const;

private:
	ui::BitmapFontLibrary m_fontLibrary;
	std::vector<RenderItem> m_items;
	std::vector<Entity> m_spriteEntities;
	std::vector<Entity> m_textEntities;

	ResourceHandle<Texture> m_whiteTexture;
	ResourceHandle<Texture> m_builtinFontTexture;
	ResourceHandle<Shader> m_defaultShader;
	std::unordered_map<int, glm::vec4> m_builtinFontGlyphUv;
	std::string m_defaultMaterialPath = "res/shaders/unlit2D.fs";
	UIRect m_screenRect{};
};
#pragma endregion
