/**
 * @file SceneEditor.h
 * @brief Declarations for SceneEditor.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include "../ecs/Entity.h"
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
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
	#pragma region Public Interface
	/**
	 * @brief Executes Draw Hierarchy.
	 */
	static void DrawHierarchy(Registry &registry, SceneEditorState &state);
	/**
	 * @brief Executes Draw Inspector.
	 */
	static void DrawInspector(Registry &registry, SceneEditorState &state);

	/**
	 * @brief Executes Create Entity.
	 */
	static Entity CreateEntity(Registry &registry, const char *name, Entity parent, bool addTransform);
	/**
	 * @brief Executes Destroy Entity Recursive.
	 */
	static void DestroyEntityRecursive(Registry &registry, Entity e);
	/**
	 * @brief Executes Duplicate Entity Recursive.
	 */
	static Entity DuplicateEntityRecursive(Registry &registry, Entity src);
	/**
	 * @brief Executes Begin Rename.
	 */
	static void BeginRename(Registry &registry, SceneEditorState &state, Entity e);
	#pragma endregion
};
#pragma endregion
