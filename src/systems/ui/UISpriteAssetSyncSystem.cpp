#pragma region Includes
#include "UISpriteAssetSyncSystem.h"

#include "../../components/ui/UISprite.h"
#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"
#include "../../utils/AssetPath.h"

#include <cstdio>
#include <functional>
#include <unordered_set>
#pragma endregion

#pragma region File Scope
namespace
{
	static std::unordered_set<std::string> s_loggedFailures;

	static std::string BuildAssetName(const char *prefix, const std::string &path)
	{
		return std::string(prefix) + std::to_string(std::hash<std::string>{}(path));
	}

	static void LogFailureOnce(const std::string &key, const std::string &message)
	{
		if (s_loggedFailures.insert(key).second)
			std::fprintf(stderr, "[UISpriteAssetSyncSystem] %s\n", message.c_str());
	}
}
#pragma endregion

#pragma region Function Definitions
void UISpriteAssetSyncSystem::Update(Registry &registry, TextureManager &textureManager, ShaderManager &shaderManager)
{
	registry.ViewEntities<UISprite>(m_entities);
	for (Entity entity : m_entities)
	{
		UISprite &sprite = registry.Get<UISprite>(entity);

		if (!sprite.texturePath.empty() && !sprite.texture.IsValid())
		{
			std::string normalizedTexturePath;
			std::string normalizeError;
			if (!AssetPath::TryNormalizeRuntimeAssetPath(sprite.texturePath, normalizedTexturePath, normalizeError))
			{
				LogFailureOnce("ui_tex_norm:" + sprite.texturePath, "Texture path '" + sprite.texturePath + "' is invalid: " + normalizeError);
			}

			std::string resolvedPath;
			std::string resolveError;
			if (!normalizedTexturePath.empty() &&
			    !AssetPath::TryResolveRuntimeFilePath(normalizedTexturePath, resolvedPath, resolveError))
			{
				LogFailureOnce("ui_tex_resolve:" + normalizedTexturePath, "Texture path '" + normalizedTexturePath + "' could not be resolved: " + resolveError);
			}

			if (!resolvedPath.empty())
			{
				auto handle = textureManager.Load(BuildAssetName("ui_tex_", normalizedTexturePath), resolvedPath, true);
				if (handle.IsValid())
				{
					sprite.texturePath = normalizedTexturePath;
					sprite.texture = handle;
				}
				else
				{
					LogFailureOnce("ui_tex_load:" + normalizedTexturePath,
					               "Failed to load texture '" + normalizedTexturePath + "' from '" + resolvedPath + "'.");
				}
			}
		}

		if (!sprite.materialPath.empty() && !sprite.shader.IsValid())
		{
			std::string normalizedMaterialPath;
			std::string normalizeError;
			if (!AssetPath::TryNormalizeRuntimeAssetPath(sprite.materialPath, normalizedMaterialPath, normalizeError))
			{
				LogFailureOnce("ui_mat_norm:" + sprite.materialPath, "Material path '" + sprite.materialPath + "' is invalid: " + normalizeError);
			}

			std::string resolvedPath;
			std::string resolveError;
			if (!normalizedMaterialPath.empty() &&
			    !AssetPath::TryResolveRuntimeFilePath(normalizedMaterialPath, resolvedPath, resolveError))
			{
				LogFailureOnce("ui_mat_resolve:" + normalizedMaterialPath, "Material path '" + normalizedMaterialPath + "' could not be resolved: " + resolveError);
			}

			std::string vsPath;
			std::string vertexError;
			if (!resolvedPath.empty() &&
			    !AssetPath::TryResolveVertexShaderPathForFragment(resolvedPath, vsPath, vertexError))
			{
				LogFailureOnce("ui_mat_vertex:" + normalizedMaterialPath, "Could not resolve vertex shader for '" + normalizedMaterialPath + "': " + vertexError);
			}

			if (!vsPath.empty())
			{
				auto handle = shaderManager.Load(BuildAssetName("ui_mat_", normalizedMaterialPath), vsPath, resolvedPath);
				if (handle.IsValid())
				{
					sprite.materialPath = normalizedMaterialPath;
					sprite.shader = handle;
				}
				else
				{
					LogFailureOnce("ui_mat_load:" + normalizedMaterialPath,
					               "Failed to load UI material shader '" + normalizedMaterialPath + "' from '" + resolvedPath + "'.");
				}
			}
		}
	}
}
#pragma endregion
