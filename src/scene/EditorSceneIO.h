/**
 * @file EditorSceneIO.h
 * @brief Helpers for saving/loading editor scenes from disk.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#include <string>
#pragma endregion

#pragma region Declarations
namespace EditorSceneIO
{
	struct SceneHeader
	{
		std::string sceneType;
		std::string sceneName;
	};

	bool PeekHeader(const std::string &path, SceneHeader &outHeader, std::string &outError);

	bool SaveRegistry(const Registry &registry,
	                  const std::string &sceneType,
	                  const std::string &sceneName,
	                  const std::string &path,
	                  std::string &outError);

	bool LoadRegistry(Registry &registry,
	                  const std::string &expectedSceneType,
	                  std::string &inOutSceneName,
	                  const std::string &path,
	                  std::string &outError);
}
#pragma endregion
