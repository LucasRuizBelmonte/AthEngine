#pragma region Includes
#include "../../platform/GL.h"
#include "UIRenderSystem.h"

#include "../../components/Parent.h"
#include "../../components/Material.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#pragma endregion

#pragma region File Scope
namespace
{
	static UIRect IntersectRect(const UIRect &a, const UIRect &b)
	{
		UIRect out;
		out.min = glm::max(a.min, b.min);
		out.max = glm::min(a.max, b.max);
		return out;
	}

	static bool RectHasArea(const UIRect &r)
	{
		const glm::vec2 size = r.Size();
		return size.x > 0.0f && size.y > 0.0f;
	}

	static glm::mat4 BuildAxisRectModel(const UIRect &rect)
	{
		const glm::vec2 size = rect.Size();
		const glm::vec2 center = (rect.min + rect.max) * 0.5f;
		const glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f));
		const glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
		return t * s;
	}

	static float Clamp01(float value)
	{
		return std::max(0.0f, std::min(1.0f, value));
	}

	static std::string ResolveRuntimeAssetPath(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();
		if (path.is_absolute())
			return path.string();

		const std::string generic = path.generic_string();
		const std::filesystem::path assetRoot(ASSET_PATH);
		const std::filesystem::path projectRoot = assetRoot.parent_path();

		if (generic == "res" || generic.rfind("res/", 0) == 0)
			return (projectRoot / path).lexically_normal().string();

		return (assetRoot / path).lexically_normal().string();
	}
}
#pragma endregion

#pragma region Function Definitions
void UIRenderSystem::Render(Registry &registry,
							Renderer &renderer,
							ShaderManager &shaderManager,
							TextureManager &textureManager,
							int framebufferWidth,
							int framebufferHeight)
{
	if (framebufferWidth <= 0 || framebufferHeight <= 0)
		return;

	EnsureResources(shaderManager, textureManager);
	BuildRenderList(registry);
	if (m_items.empty())
		return;

	m_screenRect.min = glm::vec2(0.0f, 0.0f);
	m_screenRect.max = glm::vec2(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));

	const glm::mat4 view(1.0f);
	const glm::mat4 proj = glm::ortho(0.0f,
									  static_cast<float>(framebufferWidth),
									  static_cast<float>(framebufferHeight),
									  0.0f,
									  -10.0f,
									  10.0f);
	m_batchView = view;
	m_batchProj = proj;
	m_pendingBatchVertices.clear();
	m_pendingBatchIndices.clear();
	m_hasPendingBatch = false;
	renderer.SetCamera(view, proj);

	const uint32_t quadMeshId = renderer.AcquireBuiltinQuad();
	if (quadMeshId == 0u)
		return;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (const RenderItem &item : m_items)
	{
		if (item.isText)
			RenderTextEntity(registry, renderer, quadMeshId, item.entity, framebufferHeight);
		else
			RenderSpriteEntity(registry, renderer, textureManager, quadMeshId, item.entity, framebufferHeight);
	}

	FlushPendingBatch(renderer);

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
}

void UIRenderSystem::EnsureResources(ShaderManager &shaderManager, TextureManager &textureManager)
{
	if (!m_defaultShader.IsValid())
	{
		m_defaultShader = shaderManager.Load("ui_default_unlit2d",
											 ResolveRuntimeAssetPath("res/shaders/unlit2D.vs"),
											 ResolveRuntimeAssetPath("res/shaders/unlit2D.fs"));
	}

	if (!m_batchShader.IsValid())
	{
		m_batchShader = shaderManager.Load("ui_batch_unlit",
										   ResolveRuntimeAssetPath("res/shaders/ui_batch.vs"),
										   ResolveRuntimeAssetPath("res/shaders/ui_batch.fs"));
	}

	if (!m_whiteTexture.IsValid())
		m_whiteTexture = textureManager.CreateFromRGBA("ui_white", 1, 1, {255u, 255u, 255u, 255u});

	EnsureBuiltinFontAtlas(textureManager);
}

void UIRenderSystem::EnsureBuiltinFontAtlas(TextureManager &textureManager)
{
	if (m_builtinFontTexture.IsValid() && !m_builtinFontGlyphUv.empty())
		return;

	const ui::BitmapFont *font = m_fontLibrary.GetDefaultFont();
	if (!font || font->glyphWidth <= 0 || font->glyphHeight <= 0)
		return;

	constexpr int kFirstChar = 32;
	constexpr int kCharCount = 96; // [32..127]
	constexpr int kColumns = 16;
	const int rows = (kCharCount + kColumns - 1) / kColumns;
	const int padding = 1;
	const int cellWidth = font->glyphWidth + padding * 2;
	const int cellHeight = font->glyphHeight + padding * 2;
	const int atlasWidth = kColumns * cellWidth;
	const int atlasHeight = rows * cellHeight;
	if (atlasWidth <= 0 || atlasHeight <= 0)
		return;

	std::vector<uint8_t> rgba(static_cast<size_t>(atlasWidth) * static_cast<size_t>(atlasHeight) * 4u, 0u);
	m_builtinFontGlyphUv.clear();

	for (int i = 0; i < kCharCount; ++i)
	{
		const unsigned char ch = static_cast<unsigned char>(kFirstChar + i);
		const ui::BitmapGlyph *glyph = font->FindGlyph(static_cast<char>(ch));

		const int column = i % kColumns;
		const int row = i / kColumns;
		const int cellMinX = column * cellWidth;
		const int cellMinY = row * cellHeight;

		const glm::vec4 uvRect(
			static_cast<float>(cellMinX + padding) / static_cast<float>(atlasWidth),
			static_cast<float>(cellMinY + padding) / static_cast<float>(atlasHeight),
			static_cast<float>(cellMinX + padding + font->glyphWidth) / static_cast<float>(atlasWidth),
			static_cast<float>(cellMinY + padding + font->glyphHeight) / static_cast<float>(atlasHeight));
		m_builtinFontGlyphUv[static_cast<int>(ch)] = uvRect;

		if (!glyph)
			continue;

		for (int glyphRow = 0; glyphRow < font->glyphHeight; ++glyphRow)
		{
			const uint8_t bits = glyph->rows[static_cast<size_t>(glyphRow)];
			const int dstY = cellMinY + padding + glyphRow;

			for (int glyphCol = 0; glyphCol < font->glyphWidth; ++glyphCol)
			{
				const uint8_t mask = static_cast<uint8_t>(1u << static_cast<uint8_t>(font->glyphWidth - 1 - glyphCol));
				if ((bits & mask) == 0u)
					continue;

				const int dstX = cellMinX + padding + glyphCol;
				const size_t index = (static_cast<size_t>(dstY) * static_cast<size_t>(atlasWidth) + static_cast<size_t>(dstX)) * 4u;
				rgba[index + 0u] = 255u;
				rgba[index + 1u] = 255u;
				rgba[index + 2u] = 255u;
				rgba[index + 3u] = 255u;
			}
		}
	}

	auto atlas = textureManager.CreateFromRGBA("ui_builtin_font_mono_5x7", atlasWidth, atlasHeight, std::move(rgba));
	if (!atlas.IsValid())
	{
		m_builtinFontGlyphUv.clear();
		return;
	}

	m_builtinFontTexture = atlas;
	Texture *fontTexture = textureManager.Get(m_builtinFontTexture);
	if (fontTexture && fontTexture->GetId() != 0u)
	{
		glBindTexture(GL_TEXTURE_2D, fontTexture->GetId());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

bool UIRenderSystem::TryGetBuiltinGlyphUv(char c, glm::vec4 &outUv) const
{
	auto findUv = [&](unsigned char key) -> bool
	{
		const auto it = m_builtinFontGlyphUv.find(static_cast<int>(key));
		if (it == m_builtinFontGlyphUv.end())
			return false;
		outUv = it->second;
		return true;
	};

	const unsigned char uc = static_cast<unsigned char>(c);
	if (findUv(uc))
		return true;

	if (std::islower(uc) != 0)
	{
		const unsigned char upper = static_cast<unsigned char>(std::toupper(uc));
		if (findUv(upper))
			return true;
	}

	return findUv(static_cast<unsigned char>('?'));
}

void UIRenderSystem::BuildRenderList(Registry &registry)
{
	m_items.clear();

	registry.ViewEntities<UITransform, UISprite>(m_spriteEntities);
	for (Entity entity : m_spriteEntities)
	{
		const UITransform &t = registry.Get<UITransform>(entity);
		const UISprite &sprite = registry.Get<UISprite>(entity);
		m_items.push_back(RenderItem{
			entity,
			false,
			sprite.layer,
			sprite.orderInLayer,
			t.hierarchyIndex});
	}

	registry.ViewEntities<UITransform, UIText>(m_textEntities);
	for (Entity entity : m_textEntities)
	{
		const UITransform &t = registry.Get<UITransform>(entity);
		const UIText &text = registry.Get<UIText>(entity);
		m_items.push_back(RenderItem{
			entity,
			true,
			text.layer,
			text.orderInLayer,
			t.hierarchyIndex});
	}

	std::sort(m_items.begin(), m_items.end(), [](const RenderItem &a, const RenderItem &b)
			  {
		          if (a.layer != b.layer)
			          return a.layer < b.layer;
		          if (a.orderInLayer != b.orderInLayer)
			          return a.orderInLayer < b.orderInLayer;
		          if (a.hierarchy != b.hierarchy)
			          return a.hierarchy < b.hierarchy;
		          if (a.isText != b.isText)
			          return a.isText < b.isText;
		          return a.entity < b.entity; });
}

void UIRenderSystem::RenderSpriteEntity(Registry &registry,
										Renderer &renderer,
										TextureManager &textureManager,
										uint32_t quadMeshId,
										Entity entity,
										int framebufferHeight)
{
	if (!registry.IsAlive(entity) || !registry.Has<UITransform>(entity) || !registry.Has<UISprite>(entity))
		return;

	const UITransform &transform = registry.Get<UITransform>(entity);
	const UISprite &sprite = registry.Get<UISprite>(entity);

	ResourceHandle<Texture> texture = sprite.texture.IsValid() ? sprite.texture : m_whiteTexture;
	const ResourceHandle<Shader> shader = sprite.shader.IsValid() ? sprite.shader : m_defaultShader;
	const std::string materialPath = sprite.materialPath.empty() ? m_defaultMaterialPath : sprite.materialPath;

	UIRect drawRect = transform.worldRect;
	glm::vec4 uv = sprite.uv;

	if (sprite.preserveAspect && texture.IsValid() && RectHasArea(drawRect))
	{
		Texture *tex = textureManager.Get(texture);
		if (tex && tex->GetWidth() > 0 && tex->GetHeight() > 0)
		{
			const glm::vec2 rectSize = drawRect.Size();
			const float rectAspect = rectSize.x / std::max(rectSize.y, 0.0001f);
			const float texAspect = static_cast<float>(tex->GetWidth()) / std::max(static_cast<float>(tex->GetHeight()), 0.0001f);

			glm::vec2 fitted = rectSize;
			if (rectAspect > texAspect)
			{
				fitted.x = rectSize.y * texAspect;
				fitted.y = rectSize.y;
			}
			else
			{
				fitted.x = rectSize.x;
				fitted.y = rectSize.x / texAspect;
			}

			const glm::vec2 center = (drawRect.min + drawRect.max) * 0.5f;
			drawRect.min = center - fitted * 0.5f;
			drawRect.max = drawRect.min + fitted;
		}
	}

	if (registry.Has<UIFill>(entity))
	{
		const UIFill &fill = registry.Get<UIFill>(entity);
		const float value = Clamp01(fill.value01);
		if (value <= 0.0f)
			return;

		const float width = drawRect.Size().x;
		if (width <= 0.0f)
			return;

		if (fill.direction == UIFillDirection::LeftToRight)
		{
			drawRect.max.x = drawRect.min.x + width * value;
			uv.z = uv.x + (uv.z - uv.x) * value;
		}
		else
		{
			drawRect.min.x = drawRect.max.x - width * value;
			uv.x = uv.z - (uv.z - uv.x) * value;
		}
	}

	if (!RectHasArea(drawRect))
		return;

	const std::optional<UIRect> clipRect = CollectMaskClipRect(registry, entity, m_screenRect);
	ScissorState scissor;
	if (!ResolveScissorState(clipRect, framebufferHeight, scissor))
		return;

	const glm::mat4 model = BuildModelFromAdjustedRect(transform, drawRect);
	SubmitQuad(renderer, quadMeshId, model, sprite.tint, uv, texture, shader, materialPath, scissor);
}

void UIRenderSystem::RenderTextEntity(Registry &registry,
									  Renderer &renderer,
									  uint32_t quadMeshId,
									  Entity entity,
									  int framebufferHeight)
{
	if (!registry.IsAlive(entity) || !registry.Has<UITransform>(entity) || !registry.Has<UIText>(entity))
		return;

	const UITransform &transform = registry.Get<UITransform>(entity);
	const UIText &text = registry.Get<UIText>(entity);
	if (text.text.empty())
		return;

	const ui::BitmapFont *font = m_fontLibrary.GetFont(text.fontId);
	if (!font)
		return;

	const std::optional<UIRect> clipRect = CollectMaskClipRect(registry, entity, m_screenRect);
	ScissorState scissor;
	if (!ResolveScissorState(clipRect, framebufferHeight, scissor))
		return;

	const float pixelSize = std::max(0.1f, text.fontSizePx / static_cast<float>(font->glyphHeight));
	const float advancePx = static_cast<float>(font->advance) * pixelSize;
	const float lineHeight = static_cast<float>(font->glyphHeight + 1) * pixelSize;
	const float rectWidth = transform.worldRect.Size().x;

	std::vector<std::string> inputLines;
	{
		std::string current;
		for (char c : text.text)
		{
			if (c == '\n')
			{
				inputLines.push_back(current);
				current.clear();
				continue;
			}
			current.push_back(c);
		}
		inputLines.push_back(current);
	}

	std::vector<std::string> lines;
	if (!text.wrap || advancePx <= 0.0f || rectWidth <= 0.0f)
	{
		lines = inputLines;
	}
	else
	{
		const int maxChars = std::max(1, static_cast<int>(std::floor(rectWidth / advancePx)));
		for (const std::string &raw : inputLines)
		{
			if (raw.empty())
			{
				lines.emplace_back();
				continue;
			}

			std::string remaining = raw;
			while (!remaining.empty())
			{
				if (static_cast<int>(remaining.size()) <= maxChars)
				{
					lines.push_back(remaining);
					break;
				}

				size_t split = remaining.rfind(' ', static_cast<size_t>(maxChars));
				if (split == std::string::npos || split == 0u)
					split = static_cast<size_t>(maxChars);

				lines.push_back(remaining.substr(0u, split));
				remaining = remaining.substr(std::min(split + 1u, remaining.size()));
			}
		}
	}

	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string &line = lines[i];
		const float lineWidth = static_cast<float>(line.size()) * advancePx;

		float x = transform.worldRect.min.x;
		switch (text.alignment)
		{
		case UITextAlignment::Center:
			x = transform.worldRect.min.x + (rectWidth - lineWidth) * 0.5f;
			break;
		case UITextAlignment::Right:
			x = transform.worldRect.max.x - lineWidth;
			break;
		case UITextAlignment::Left:
		default:
			x = transform.worldRect.min.x;
			break;
		}

		const float y = transform.worldRect.min.y + static_cast<float>(i) * lineHeight;
		DrawTextLineGlyphs(renderer, quadMeshId, transform, text, *font, line, x, y, scissor);
	}
}

std::optional<UIRect> UIRenderSystem::CollectMaskClipRect(Registry &registry, Entity entity, const UIRect &screenRect) const
{
	UIRect clip = screenRect;
	bool hasMask = false;

	Entity current = entity;
	for (int depth = 0; depth < 256; ++depth)
	{
		if (!registry.IsAlive(current) || !registry.Has<Parent>(current))
			break;

		const Entity parent = registry.Get<Parent>(current).parent;
		if (parent == kInvalidEntity || !registry.IsAlive(parent))
			break;

		if (registry.Has<UIMask>(parent) && registry.Has<UITransform>(parent))
		{
			const UIMask &mask = registry.Get<UIMask>(parent);
			if (mask.enabled)
			{
				hasMask = true;
				clip = IntersectRect(clip, registry.Get<UITransform>(parent).worldRect);
			}
		}

		current = parent;
	}

	if (!hasMask)
		return std::nullopt;

	return clip;
}

bool UIRenderSystem::ResolveScissorState(const std::optional<UIRect> &clipRect,
										 int framebufferHeight,
										 ScissorState &outScissor) const
{
	if (!clipRect.has_value())
	{
		outScissor = ScissorState{};
		return true;
	}

	UIRect clipped = IntersectRect(*clipRect, m_screenRect);
	if (!RectHasArea(clipped))
		return false;

	const int x = static_cast<int>(std::floor(clipped.min.x));
	const int y = static_cast<int>(std::floor(static_cast<float>(framebufferHeight) - clipped.max.y));
	const int width = static_cast<int>(std::ceil(clipped.max.x) - std::floor(clipped.min.x));
	const int height = static_cast<int>(std::ceil(clipped.max.y) - std::floor(clipped.min.y));

	if (width <= 0 || height <= 0)
		return false;

	outScissor.enabled = true;
	outScissor.x = x;
	outScissor.y = y;
	outScissor.width = width;
	outScissor.height = height;
	return true;
}

void UIRenderSystem::ApplyScissorState(const ScissorState &scissor) const
{
	if (!scissor.enabled)
	{
		glDisable(GL_SCISSOR_TEST);
		return;
	}

	glEnable(GL_SCISSOR_TEST);
	glScissor(scissor.x, scissor.y, scissor.width, scissor.height);
}

bool UIRenderSystem::ScissorEquals(const ScissorState &a, const ScissorState &b) const
{
	return a.enabled == b.enabled &&
		   a.x == b.x &&
		   a.y == b.y &&
		   a.width == b.width &&
		   a.height == b.height;
}

void UIRenderSystem::FlushPendingBatch(Renderer &renderer)
{
	if (!m_hasPendingBatch || m_pendingBatchVertices.empty() || m_pendingBatchIndices.empty())
		return;

	ApplyScissorState(m_pendingBatchKey.scissor);
	renderer.SubmitUnlitQuadBatch(m_batchShader,
								  m_pendingBatchKey.texture,
								  m_pendingBatchVertices,
								  m_pendingBatchIndices,
								  m_batchView,
								  m_batchProj);

	m_pendingBatchVertices.clear();
	m_pendingBatchIndices.clear();
	m_hasPendingBatch = false;
}

void UIRenderSystem::AppendQuadToBatch(Renderer &renderer,
									   const glm::mat4 &model,
									   const glm::vec4 &tint,
									   const glm::vec4 &uv,
									   const ResourceHandle<Texture> &texture,
									   const ScissorState &scissor)
{
	const bool textureChanged = !m_hasPendingBatch || (m_pendingBatchKey.texture.id != texture.id);
	const bool scissorChanged = !m_hasPendingBatch || !ScissorEquals(m_pendingBatchKey.scissor, scissor);
	const bool overflow = m_pendingBatchVertices.size() + 4u > 65536u;
	if (textureChanged || scissorChanged || overflow)
	{
		FlushPendingBatch(renderer);
		m_pendingBatchKey.texture = texture;
		m_pendingBatchKey.scissor = scissor;
		m_hasPendingBatch = true;
	}

	const uint32_t base = static_cast<uint32_t>(m_pendingBatchVertices.size());

	const glm::vec4 p0 = model * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f);
	const glm::vec4 p1 = model * glm::vec4(0.5f, -0.5f, 0.0f, 1.0f);
	const glm::vec4 p2 = model * glm::vec4(0.5f, 0.5f, 0.0f, 1.0f);
	const glm::vec4 p3 = model * glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f);
	const glm::vec2 positions[4] = {
		glm::vec2(p0.x, p0.y),
		glm::vec2(p1.x, p1.y),
		glm::vec2(p2.x, p2.y),
		glm::vec2(p3.x, p3.y),
	};

	const glm::vec2 uvs[4] = {
		glm::vec2(uv.x, uv.y),
		glm::vec2(uv.z, uv.y),
		glm::vec2(uv.z, uv.w),
		glm::vec2(uv.x, uv.w),
	};

	for (int i = 0; i < 4; ++i)
	{
		Renderer::UnlitBatchVertex vertex;
		vertex.position = positions[i];
		vertex.uv = uvs[i];
		vertex.color = tint;
		m_pendingBatchVertices.push_back(vertex);
	}

	m_pendingBatchIndices.push_back(base + 0u);
	m_pendingBatchIndices.push_back(base + 1u);
	m_pendingBatchIndices.push_back(base + 2u);
	m_pendingBatchIndices.push_back(base + 2u);
	m_pendingBatchIndices.push_back(base + 3u);
	m_pendingBatchIndices.push_back(base + 0u);
}

glm::mat4 UIRenderSystem::BuildModelFromAdjustedRect(const UITransform &transform, const UIRect &adjustedRect) const
{
	const glm::vec2 originalSize = transform.worldRect.Size();
	const glm::vec2 newSize = adjustedRect.Size();
	if (originalSize.x <= 0.0001f || originalSize.y <= 0.0001f)
		return transform.worldMatrix;

	const glm::vec2 originalCenter = (transform.worldRect.min + transform.worldRect.max) * 0.5f;
	const glm::vec2 newCenter = (adjustedRect.min + adjustedRect.max) * 0.5f;
	const glm::vec2 delta = newCenter - originalCenter;
	const glm::vec2 scaleFactor(newSize.x / originalSize.x, newSize.y / originalSize.y);

	const glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(delta, 0.0f));
	const glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor, 1.0f));
	return translate * transform.worldMatrix * scale;
}

void UIRenderSystem::SubmitQuad(Renderer &renderer,
								uint32_t quadMeshId,
								const glm::mat4 &model,
								const glm::vec4 &tint,
								const glm::vec4 &uv,
								const ResourceHandle<Texture> &texture,
								const ResourceHandle<Shader> &shader,
								const std::string &materialPath,
								const ScissorState &scissor)
{
	const bool useFastUnlitPath =
		shader.id == m_defaultShader.id &&
		materialPath == m_defaultMaterialPath;
	if (useFastUnlitPath && m_batchShader.IsValid())
	{
		AppendQuadToBatch(renderer, model, tint, uv, texture, scissor);
		return;
	}

	FlushPendingBatch(renderer);
	ApplyScissorState(scissor);

	if (useFastUnlitPath)
	{
		renderer.SubmitUnlitQuad(quadMeshId, shader, texture, model, tint, uv);
		return;
	}

	Material material;
	material.shader = shader;
	material.shaderPath = materialPath;
	material.parameters.reserve(3u);

	MaterialParameter tintParam;
	tintParam.type = MaterialParameterType::Vec4;
	tintParam.numericValue = tint;
	material.parameters.emplace("u_tint", std::move(tintParam));

	MaterialParameter uvParam;
	uvParam.type = MaterialParameterType::Vec4;
	uvParam.numericValue = uv;
	material.parameters.emplace("u_uvRect", std::move(uvParam));

	MaterialParameter texParam;
	texParam.type = MaterialParameterType::Texture2D;
	texParam.texture = texture;
	material.parameters.emplace("u_texBaseColor", std::move(texParam));

	renderer.SubmitMesh(quadMeshId, material, model);
}

void UIRenderSystem::DrawTextLineGlyphs(Renderer &renderer,
										uint32_t quadMeshId,
										const UITransform &,
										const UIText &text,
										const ui::BitmapFont &font,
										const std::string &line,
										float originX,
										float originY,
										const ScissorState &scissor)
{
	const bool useAtlasPath =
		m_builtinFontTexture.IsValid() &&
		!m_builtinFontGlyphUv.empty() &&
		font.id == "builtin_mono_5x7";
	const float rawPixelSize = std::max(0.1f, text.fontSizePx / static_cast<float>(font.glyphHeight));
	const float pixelSize = useAtlasPath ? std::max(1.0f, std::round(rawPixelSize)) : rawPixelSize;
	const float advancePx = static_cast<float>(font.advance) * pixelSize;

	auto drawPass = [&](const glm::vec4 &color, const glm::vec2 &offset)
	{
		float penX = originX;
		for (char c : line)
		{
			if (useAtlasPath)
			{
				if (c != ' ')
				{
					glm::vec4 uv;
					if (TryGetBuiltinGlyphUv(c, uv))
					{
						UIRect glyphRect;
						glyphRect.min = glm::round(glm::vec2(penX + offset.x, originY + offset.y));
						glyphRect.max = glyphRect.min + glm::vec2(
															static_cast<float>(font.glyphWidth) * pixelSize,
															static_cast<float>(font.glyphHeight) * pixelSize);

						const glm::mat4 model = BuildAxisRectModel(glyphRect);
						SubmitQuad(renderer,
								   quadMeshId,
								   model,
								   color,
								   uv,
								   m_builtinFontTexture,
								   m_defaultShader,
								   m_defaultMaterialPath,
								   scissor);
					}
				}
			}
			else
			{
				const ui::BitmapGlyph *glyph = font.FindGlyph(c);
				if (glyph)
				{
					for (int row = 0; row < font.glyphHeight; ++row)
					{
						const uint8_t bits = glyph->rows[static_cast<size_t>(row)];
						for (int col = 0; col < font.glyphWidth; ++col)
						{
							const uint8_t mask = static_cast<uint8_t>(1u << static_cast<uint8_t>(font.glyphWidth - 1 - col));
							if ((bits & mask) == 0u)
								continue;

							UIRect pixelRect;
							pixelRect.min = glm::vec2(
								penX + static_cast<float>(col) * pixelSize + offset.x,
								originY + static_cast<float>(row) * pixelSize + offset.y);
							pixelRect.max = pixelRect.min + glm::vec2(pixelSize, pixelSize);

							const glm::mat4 model = BuildAxisRectModel(pixelRect);
							SubmitQuad(renderer,
									   quadMeshId,
									   model,
									   color,
									   glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
									   m_whiteTexture,
									   m_defaultShader,
									   m_defaultMaterialPath,
									   scissor);
						}
					}
				}
			}

			penX += advancePx;
		}
	};

	if (text.outlineEnabled && text.outlineThicknessPx > 0.0f)
	{
		const float t = text.outlineThicknessPx;
		const glm::vec2 offsets[] = {
			glm::vec2(-t, 0.0f),
			glm::vec2(t, 0.0f),
			glm::vec2(0.0f, -t),
			glm::vec2(0.0f, t),
			glm::vec2(-t, -t),
			glm::vec2(t, -t),
			glm::vec2(-t, t),
			glm::vec2(t, t)};

		for (const glm::vec2 &offset : offsets)
			drawPass(text.outlineColor, offset);
	}

	drawPass(text.color, glm::vec2(0.0f, 0.0f));
}
#pragma endregion
