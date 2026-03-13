#include "AthSceneIOInternal.h"

#include "../utils/AssetPath.h"

#include <filesystem>

namespace AthSceneIO
{
	std::string ScenePathResolver::ToRelativePathForSave(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();

		if (path.is_absolute())
		{
			std::string runtimeAssetRootText;
			std::string runtimeAssetRootError;
			const bool hasRuntimeAssetRoot = AssetPath::TryGetRuntimeAssetRoot(runtimeAssetRootText, runtimeAssetRootError);
			const std::filesystem::path runtimeAssetRoot =
				hasRuntimeAssetRoot ? std::filesystem::path(runtimeAssetRootText)
				                    : std::filesystem::path(ASSET_PATH).lexically_normal();

			std::error_code ec;
			std::filesystem::path rel = std::filesystem::relative(path, runtimeAssetRoot, ec);
			if (!ec && !rel.empty())
			{
				path = rel.lexically_normal();
			}
			else
			{
				ec.clear();
				const std::filesystem::path cwd = std::filesystem::current_path(ec);
				if (!ec)
				{
					rel = std::filesystem::relative(path, cwd, ec);
					if (!ec && !rel.empty())
						path = rel.lexically_normal();
				}
			}
		}

		std::string candidate = path.generic_string();
		if (candidate.rfind("res/", 0) == 0)
			candidate = candidate.substr(4u);

		std::string normalized;
		std::string normalizeError;
		if (AssetPath::TryNormalizeRuntimeAssetPath(candidate, normalized, normalizeError))
			return normalized;

		return std::filesystem::path(candidate).lexically_normal().generic_string();
	}
}
