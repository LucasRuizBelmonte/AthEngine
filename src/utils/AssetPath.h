/**
 * @file AssetPath.h
 * @brief Shared helpers for resolving runtime asset file paths.
 */

#pragma once

#include <string>

namespace AssetPath
{
	bool TryGetRuntimeAssetRoot(std::string &outRootPath, std::string &outError);
	bool TryNormalizeRuntimeAssetPath(const std::string &rawPath, std::string &outNormalizedPath, std::string &outError);
	bool TryResolveRuntimePath(const std::string &rawPath, std::string &outResolvedPath, std::string &outError);
	bool TryResolveRuntimeFilePath(const std::string &rawPath, std::string &outResolvedPath, std::string &outError);
	std::string ResolveRuntimePath(const std::string &rawPath);
	bool TryResolveVertexShaderPathForFragment(const std::string &fragmentPath, std::string &outVertexPath, std::string &outError);
	std::string ResolveVertexShaderPathForFragment(const std::string &fragmentPath);
	bool TryResolveMaterialMetadataPathForFragment(const std::string &fragmentPath, std::string &outMetadataPath, std::string &outError);
	std::string ResolveMaterialMetadataPathForFragment(const std::string &fragmentPath);
}
