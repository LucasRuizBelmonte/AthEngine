#pragma region Includes
#include "UISpriteAssetSyncSystem.h"

#include "../../components/ui/UISprite.h"
#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"
#include "../../utils/AssetPath.h"

#include <functional>
#pragma endregion

#pragma region File Scope
namespace
{
	static std::string BuildAssetName(const char *prefix, const std::string &path)
	{
		return std::string(prefix) + std::to_string(std::hash<std::string>{}(path));
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
			const std::string resolvedPath = AssetPath::ResolveRuntimePath(sprite.texturePath);
			auto handle = textureManager.Load(BuildAssetName("ui_tex_", sprite.texturePath), resolvedPath, true);
			if (handle.IsValid())
				sprite.texture = handle;
		}

		if (!sprite.materialPath.empty() && !sprite.shader.IsValid())
		{
			const std::string resolvedPath = AssetPath::ResolveRuntimePath(sprite.materialPath);
			const std::string vsPath = AssetPath::ResolveVertexShaderPathForFragment(resolvedPath);
			if (!vsPath.empty())
			{
				auto handle = shaderManager.Load(BuildAssetName("ui_mat_", sprite.materialPath), vsPath, resolvedPath);
				if (handle.IsValid())
					sprite.shader = handle;
			}
		}
	}
}
#pragma endregion
