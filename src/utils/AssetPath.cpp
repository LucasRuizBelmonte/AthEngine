#include "AssetPath.h"

#include <filesystem>

namespace AssetPath
{
	std::string ResolveRuntimePath(const std::string &rawPath)
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

	std::string ResolveVertexShaderPathForFragment(const std::string &fragmentPath)
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

	std::string ResolveMaterialMetadataPathForFragment(const std::string &fragmentPath)
	{
		if (fragmentPath.empty())
			return {};

		std::filesystem::path fragmentFsPath(fragmentPath);
		std::error_code ec;

		std::filesystem::path candidate = fragmentFsPath;
		candidate.replace_extension(".matmeta");
		if (std::filesystem::exists(candidate, ec))
			return candidate.lexically_normal().string();
		ec.clear();

		candidate = fragmentFsPath;
		candidate += ".matmeta";
		if (std::filesystem::exists(candidate, ec))
			return candidate.lexically_normal().string();

		return {};
	}
}
