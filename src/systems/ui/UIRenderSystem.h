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
#include <string_view>
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

	struct ScissorState
	{
		bool enabled = false;
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};

	struct BatchKey
	{
		ResourceHandle<Texture> texture;
		ScissorState scissor;
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
	bool ResolveScissorState(const std::optional<UIRect> &clipRect, int framebufferHeight, ScissorState &outScissor) const;
	void ApplyScissorState(const ScissorState &scissor) const;
	void FlushPendingBatch(Renderer &renderer);
	void AppendQuadToBatch(Renderer &renderer,
	                      const glm::mat4 &model,
	                      const glm::vec4 &tint,
	                      const glm::vec4 &uv,
	                      const ResourceHandle<Texture> &texture,
	                      const ScissorState &scissor);
	bool ScissorEquals(const ScissorState &a, const ScissorState &b) const;

	glm::mat4 BuildModelFromAdjustedRect(const UITransform &transform, const UIRect &adjustedRect) const;
	void SubmitQuad(Renderer &renderer,
	                uint32_t quadMeshId,
	                const glm::mat4 &model,
	                const glm::vec4 &tint,
	                const glm::vec4 &uv,
	                const ResourceHandle<Texture> &texture,
	                const ResourceHandle<Shader> &shader,
	                const std::string &materialPath,
	                const ScissorState &scissor);

	void DrawTextLineGlyphs(Renderer &renderer,
	                        uint32_t quadMeshId,
	                        const UITransform &transform,
	                        const UIText &text,
	                        const ui::BitmapFont &font,
	                        std::string_view line,
	                        float originX,
	                        float originY,
	                        const ScissorState &scissor);
	bool TryGetBuiltinGlyphUv(char c, glm::vec4 &outUv) const;

private:
	ui::BitmapFontLibrary m_fontLibrary;
	std::vector<RenderItem> m_items;
	std::vector<Entity> m_spriteEntities;
	std::vector<Entity> m_textEntities;

	ResourceHandle<Texture> m_whiteTexture;
	ResourceHandle<Texture> m_builtinFontTexture;
	ResourceHandle<Shader> m_defaultShader;
	ResourceHandle<Shader> m_batchShader;
	std::unordered_map<int, glm::vec4> m_builtinFontGlyphUv;
	std::string m_defaultMaterialPath = "res/shaders/unlit2D.fs";
	UIRect m_screenRect{};

	std::vector<Renderer::UnlitBatchVertex> m_pendingBatchVertices;
	std::vector<uint32_t> m_pendingBatchIndices;
	BatchKey m_pendingBatchKey{};
	bool m_hasPendingBatch = false;
	glm::mat4 m_batchView{1.0f};
	glm::mat4 m_batchProj{1.0f};
	std::vector<std::string_view> m_textInputLineViews;
	std::vector<std::string_view> m_textWrappedLineViews;
};
#pragma endregion
