#pragma once
#include <cstdint>
#include "../ecs/Entity.h"
#include "../ecs/Registry.h"

struct SceneEditorState
{
	Entity selectedEntity = kInvalidEntity;
	Entity renamingEntity = kInvalidEntity;
	bool renameFocus = false;
	char renameBuf[256] = {};
};

class SceneEditor
{
public:
	static void DrawHierarchy(Registry &registry, SceneEditorState &state);
	static void DrawInspector(Registry &registry, SceneEditorState &state);

	static Entity CreateEntity(Registry &registry, const char *name, Entity parent, bool addTransform);
	static void DestroyEntityRecursive(Registry &registry, Entity e);
	static Entity DuplicateEntityRecursive(Registry &registry, Entity src);
	static void BeginRename(Registry &registry, SceneEditorState &state, Entity e);
};