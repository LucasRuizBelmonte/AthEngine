#pragma region Includes
#include "EditorSceneIO.h"

#include "AthSceneIO.h"
#pragma endregion

#pragma region Function Definitions
bool EditorSceneIO::PeekHeader(const std::string &path, SceneHeader &outHeader, std::string &outError)
{
	AthSceneIO::SceneHeader header;
	const bool ok = AthSceneIO::AthSceneReader::PeekHeader(path, header, outError);
	if (!ok)
		return false;

	outHeader.sceneType = header.sceneType;
	outHeader.sceneName = header.sceneName;
	return true;
}

bool EditorSceneIO::SaveRegistry(const Registry &registry,
                                 const std::string &sceneType,
                                 const std::string &sceneName,
                                 const std::string &path,
                                 std::string &outError)
{
	return AthSceneIO::AthSceneWriter::SaveRegistry(registry, sceneType, sceneName, path, outError);
}

bool EditorSceneIO::LoadRegistry(Registry &registry,
                                 const std::string &expectedSceneType,
                                 std::string &inOutSceneName,
                                 const std::string &path,
                                 std::string &outError)
{
	return AthSceneIO::AthSceneReader::LoadRegistry(registry, expectedSceneType, inOutSceneName, path, outError);
}
#pragma endregion
