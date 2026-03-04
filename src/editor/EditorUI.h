/**
 * @file EditorUI.h
 * @brief Declarations for EditorUI.
 */

#pragma once

#pragma region Includes
#include <cstddef>
#include <string>
#include <imgui.h>
#include "../ecs/Entity.h"
#pragma endregion

#pragma region Declarations
class SceneManager;

struct EditorUIState
{
	bool showSceneList = true;
	bool showEntityHierarchy = true;
	bool showSystems = true;
	bool showInspector = true;
	bool showRender = true;

	size_t selectedScene = 0;

	bool dockLayoutBuilt = false;

	size_t renamingSceneIndex = static_cast<size_t>(-1);
	char renameSceneBuf[128] = {};

	bool saveScenePopupOpen = false;
	bool openScenePopupOpen = false;
	char savePathBuf[260] = "res/scenes/scene.athscene";
	char openPathBuf[260] = "res/scenes/scene.athscene";
	std::string sceneFileStatus;
};

class EditorUI
{
public:
	#pragma region Public Interface
	/**
	 * @brief Executes Set Render Texture.
	 */
	static void SetRenderTexture(ImTextureID textureId);
	/**
	 * @brief Executes Get Render Target Size.
	 */
	static ImVec2 GetRenderTargetSize();
	/**
	 * @brief Returns whether the mouse is currently over the Render window.
	 */
	static bool IsRenderWindowHovered();

	/**
	 * @brief Executes Draw.
	 */
	static void Draw(SceneManager &scenes, EditorUIState &state);
	#pragma endregion
};
#pragma endregion
