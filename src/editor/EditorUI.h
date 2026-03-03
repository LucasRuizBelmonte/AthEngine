#pragma once
#include <cstddef>
#include "../ecs/Entity.h"

class SceneManager;

struct EditorUIState
{
	bool showSceneList = true;
	bool showEntityHierarchy = true;
	bool showSystems = true;
	bool showInspector = true;

	size_t selectedScene = 0;

#ifdef IMGUI_HAS_DOCK
	bool dockLayoutBuilt = false;
#endif
};

class EditorUI
{
public:
	static void Draw(SceneManager &scenes, EditorUIState &state);
};