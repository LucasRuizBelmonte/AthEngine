#pragma once
#include <cstddef>
#include <imgui.h>
#include "../ecs/Entity.h"

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
};

class EditorUI
{
public:
	static void Draw(SceneManager &scenes, EditorUIState &state);
};
