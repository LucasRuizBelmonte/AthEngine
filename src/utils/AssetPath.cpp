#include "AssetPath.h"

#include <array>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace AssetPath
{
	namespace
	{
		bool IsExistingDirectory(const std::filesystem::path &path)
		{
			std::error_code ec;
			return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
		}

		bool IsRuntimeAssetRoot(const std::filesystem::path &path)
		{
			if (!IsExistingDirectory(path))
				return false;

			std::error_code ec;
			return std::filesystem::exists(path / "runtime" / "startup.cfg", ec);
		}

		std::filesystem::path ResolveAssetRoot()
		{
			const char *assetPathOverride = std::getenv("ATH_ASSET_PATH");
			if (assetPathOverride != nullptr && assetPathOverride[0] != '\0')
				return std::filesystem::path(assetPathOverride).lexically_normal();

#ifdef _WIN32
			std::array<char, MAX_PATH> buffer{};
			const DWORD copied = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (copied > 0u && copied < buffer.size())
			{
				const std::filesystem::path executablePath(buffer.data());
				const std::filesystem::path packagedAssetRoot =
					(executablePath.parent_path() / "res").lexically_normal();
				if (IsRuntimeAssetRoot(packagedAssetRoot))
					return packagedAssetRoot;
			}
#endif

			return std::filesystem::path(ASSET_PATH).lexically_normal();
		}
	}

	std::string ResolveRuntimePath(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();
		if (path.is_absolute())
			return path.string();

		const std::filesystem::path assetRoot = ResolveAssetRoot();
		const std::string generic = path.generic_string();
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
