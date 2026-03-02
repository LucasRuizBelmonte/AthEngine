#pragma once
#include <unordered_map>
#include <vector>
#include "../ecs/Entity.h"
#include "../ecs/Registry.h"

class SceneEditor
{
public:
	static void DrawEntityHierarchy(Registry &registry, Entity &selectedEntity);
	static void DrawInspector(Registry &registry, Entity selectedEntity);

private:
	static void BuildChildrenMap(
		Registry &registry,
		std::unordered_map<Entity, std::vector<Entity>> &outChildren,
		std::vector<Entity> &outRoots);

	static void DrawEntityNode(
		Registry &registry,
		Entity e,
		const std::unordered_map<Entity, std::vector<Entity>> &children,
		Entity &selectedEntity);
};