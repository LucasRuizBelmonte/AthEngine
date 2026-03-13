#include "AssetPath.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string_view>

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
		static constexpr std::array<std::string_view, 8u> kRuntimeAssetTopLevelDirs = {
			"runtime",
			"scenes",
			"shaders",
			"textures",
			"audio",
			"fonts",
			"models",
			"data"};

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

		bool IsAllowedTopLevelDir(const std::string &segment)
		{
			for (const std::string_view candidate : kRuntimeAssetTopLevelDirs)
			{
				if (segment == candidate)
					return true;
			}
			return false;
		}

		bool TryGetExecutableAssetRoot(std::filesystem::path &outRoot)
		{
#ifdef _WIN32
			std::array<char, MAX_PATH> buffer{};
			const DWORD copied = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (copied == 0u || copied >= buffer.size())
				return false;

			const std::filesystem::path executablePath(buffer.data());
			const std::filesystem::path packagedAssetRoot =
				(executablePath.parent_path() / "assets").lexically_normal();
			if (!IsRuntimeAssetRoot(packagedAssetRoot))
				return false;

			outRoot = packagedAssetRoot;
			return true;
#else
			(void)outRoot;
			return false;
#endif
		}

		bool TryGetConfiguredAssetRoot(std::filesystem::path &outRoot, std::string &outError)
		{
			const char *assetPathOverride = std::getenv("ATH_ASSET_PATH");
			if (assetPathOverride != nullptr && assetPathOverride[0] != '\0')
			{
				const std::filesystem::path overrideRoot = std::filesystem::path(assetPathOverride).lexically_normal();
				if (!IsRuntimeAssetRoot(overrideRoot))
				{
					outError = "ATH_ASSET_PATH is set to '" + overrideRoot.generic_string() +
					           "', but runtime/startup.cfg was not found there.";
					return false;
				}

				outRoot = overrideRoot;
				return true;
			}

			if (TryGetExecutableAssetRoot(outRoot))
				return true;

#if defined(ATH_RUNTIME_BUILD)
			outError = "Could not locate packaged runtime assets folder. Expected 'assets/runtime/startup.cfg' next to the runtime executable, or set ATH_ASSET_PATH.";
			return false;
#else
			const std::filesystem::path sourceAssetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
			if (!IsRuntimeAssetRoot(sourceAssetRoot))
			{
				outError = "Could not locate a valid asset root. Checked ATH_ASSET_PATH, executable 'assets' folder, and ASSET_PATH.";
				return false;
			}

			outRoot = sourceAssetRoot;
			return true;
#endif
		}
	}

	bool TryGetRuntimeAssetRoot(std::string &outRootPath, std::string &outError)
	{
		std::filesystem::path root;
		if (!TryGetConfiguredAssetRoot(root, outError))
			return false;

		outRootPath = root.lexically_normal().string();
		return true;
	}

	bool TryNormalizeRuntimeAssetPath(const std::string &rawPath, std::string &outNormalizedPath, std::string &outError)
	{
		if (rawPath.empty())
		{
			outError = "Asset path is empty.";
			return false;
		}

		std::filesystem::path normalizedPath = std::filesystem::path(rawPath).lexically_normal();
		if (normalizedPath.empty() || normalizedPath == std::filesystem::path("."))
		{
			outError = "Asset path '" + rawPath + "' is not a valid runtime asset path.";
			return false;
		}

		if (normalizedPath.is_absolute() || normalizedPath.has_root_directory() || normalizedPath.has_root_name())
		{
			outError = "Runtime asset paths must be relative and rooted at a runtime asset directory. Invalid path: '" + rawPath + "'.";
			return false;
		}

		for (const auto &part : normalizedPath)
		{
			if (part == "..")
			{
				outError = "Runtime asset path cannot contain '..': '" + rawPath + "'.";
				return false;
			}
		}

		const auto it = normalizedPath.begin();
		if (it == normalizedPath.end())
		{
			outError = "Asset path '" + rawPath + "' is not a valid runtime asset path.";
			return false;
		}

		const std::string topLevel = (*it).generic_string();
		if (topLevel == "res")
		{
			outError = "Legacy 'res/' runtime prefix is not supported. Use '<type>/...' paths (for example 'textures/sprite.png').";
			return false;
		}
		if (topLevel == "assets")
		{
			outError = "Runtime asset paths must not include the packaged root folder name ('assets/').";
			return false;
		}
		if (!IsAllowedTopLevelDir(topLevel))
		{
			outError = "Unsupported runtime asset path root '" + topLevel + "' in '" + rawPath +
			           "'. Allowed roots: runtime, scenes, shaders, textures, audio, fonts, models, data.";
			return false;
		}

		outNormalizedPath = normalizedPath.generic_string();
		return true;
	}

	bool TryResolveRuntimePath(const std::string &rawPath, std::string &outResolvedPath, std::string &outError)
	{
		if (rawPath.empty())
		{
			outError = "Asset path is empty.";
			return false;
		}

		const std::filesystem::path inputPath(rawPath);
		if (inputPath.is_absolute())
		{
			outResolvedPath = inputPath.lexically_normal().string();
			return true;
		}

		std::string normalizedAssetPath;
		if (!TryNormalizeRuntimeAssetPath(rawPath, normalizedAssetPath, outError))
			return false;

		std::string runtimeRootPath;
		if (!TryGetRuntimeAssetRoot(runtimeRootPath, outError))
			return false;

		const std::filesystem::path resolved = std::filesystem::path(runtimeRootPath) / std::filesystem::path(normalizedAssetPath);
		outResolvedPath = resolved.lexically_normal().string();
		return true;
	}

	bool TryResolveRuntimeFilePath(const std::string &rawPath, std::string &outResolvedPath, std::string &outError)
	{
		if (!TryResolveRuntimePath(rawPath, outResolvedPath, outError))
			return false;

		std::error_code ec;
		const std::filesystem::path filePath(outResolvedPath);
		if (!std::filesystem::exists(filePath, ec))
		{
			outError = "Runtime asset file not found: '" + rawPath + "' (resolved to '" + filePath.generic_string() + "').";
			return false;
		}
		if (!std::filesystem::is_regular_file(filePath, ec))
		{
			outError = "Runtime asset path is not a file: '" + rawPath + "' (resolved to '" + filePath.generic_string() + "').";
			return false;
		}

		return true;
	}

	std::string ResolveRuntimePath(const std::string &rawPath)
	{
		std::string resolved;
		std::string error;
		if (!TryResolveRuntimePath(rawPath, resolved, error))
		{
			if (!error.empty())
				std::fprintf(stderr, "[AssetPath] %s\n", error.c_str());
			return {};
		}
		return resolved;
	}

	bool TryResolveVertexShaderPathForFragment(const std::string &fragmentPath, std::string &outVertexPath, std::string &outError)
	{
		if (fragmentPath.empty())
		{
			outError = "Fragment shader path is empty.";
			return false;
		}

		std::filesystem::path candidate(fragmentPath);
		if (!candidate.has_extension())
		{
			outError = "Fragment shader path has no extension: '" + fragmentPath + "'.";
			return false;
		}

		candidate.replace_extension(".vs");
		std::error_code ec;
		if (!std::filesystem::exists(candidate, ec) || !std::filesystem::is_regular_file(candidate, ec))
		{
			outError = "Missing vertex shader companion for fragment shader '" + fragmentPath +
			           "'. Expected: '" + candidate.generic_string() + "'.";
			return false;
		}

		outVertexPath = candidate.lexically_normal().string();
		return true;
	}

	std::string ResolveVertexShaderPathForFragment(const std::string &fragmentPath)
	{
		std::string vertexPath;
		std::string error;
		if (!TryResolveVertexShaderPathForFragment(fragmentPath, vertexPath, error))
			return {};
		return vertexPath;
	}

	bool TryResolveMaterialMetadataPathForFragment(const std::string &fragmentPath, std::string &outMetadataPath, std::string &outError)
	{
		if (fragmentPath.empty())
		{
			outError = "Fragment shader path is empty.";
			return false;
		}

		std::filesystem::path fragmentFsPath(fragmentPath);
		std::error_code ec;

		std::filesystem::path candidate = fragmentFsPath;
		candidate.replace_extension(".matmeta");
		if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec))
		{
			outMetadataPath = candidate.lexically_normal().string();
			return true;
		}
		ec.clear();

		candidate = fragmentFsPath;
		candidate += ".matmeta";
		if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec))
		{
			outMetadataPath = candidate.lexically_normal().string();
			return true;
		}

		outError = "Missing material metadata companion for fragment shader '" + fragmentPath + "'.";
		return false;
	}

	std::string ResolveMaterialMetadataPathForFragment(const std::string &fragmentPath)
	{
		std::string metadataPath;
		std::string error;
		if (!TryResolveMaterialMetadataPathForFragment(fragmentPath, metadataPath, error))
			return {};
		return metadataPath;
	}
}
