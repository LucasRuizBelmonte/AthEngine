#pragma region Includes
#include "UISpriteAssetSyncSystem.h"

#include "../../components/ui/UISprite.h"
#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"

#include <filesystem>
#include <functional>
#pragma endregion

#pragma region File Scope
namespace
{
	static std::string BuildAssetName(const char *prefix, const std::string &path)
	{
		return std::string(prefix) + std::to_string(std::hash<std::string>{}(path));
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

	static std::string ResolveVertexShaderPathForFragment(const std::string &fragmentPath)
	{
		if (fragmentPath.empty())
			return {};

		std::filesystem::path candidate(fragmentPath);
		if (!candidate.has_extension())
			return {};

		candidate.replace_extension(".vs");
		std::error_code ec;
		if (std::filesystem::exists(candidate, ec))
			return candidate.string();

		return {};
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
			const std::string resolvedPath = ResolveRuntimeAssetPath(sprite.texturePath);
			auto handle = textureManager.Load(BuildAssetName("ui_tex_", sprite.texturePath), resolvedPath, true);
			if (handle.IsValid())
				sprite.texture = handle;
		}

		if (!sprite.materialPath.empty() && !sprite.shader.IsValid())
		{
			const std::string resolvedPath = ResolveRuntimeAssetPath(sprite.materialPath);
			const std::string vsPath = ResolveVertexShaderPathForFragment(resolvedPath);
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
