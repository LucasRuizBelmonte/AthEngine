/**
 * @file AssetPath.h
 * @brief Shared helpers for resolving runtime asset file paths.
 */

#pragma once

#include <string>

namespace AssetPath
{
	std::string ResolveRuntimePath(const std::string &rawPath);
	std::string ResolveVertexShaderPathForFragment(const std::string &fragmentPath);
	std::string ResolveMaterialMetadataPathForFragment(const std::string &fragmentPath);
}
