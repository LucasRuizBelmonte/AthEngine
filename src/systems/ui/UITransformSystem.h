/**
 * @file UITransformSystem.h
 * @brief Declarations for UITransformSystem.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Registry.h"
#include "../../components/ui/UIComponents.h"

#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region Declarations
class UITransformSystem
{
public:
	void Update(Registry &registry, int framebufferWidth, int framebufferHeight);

private:
	using ChildrenMap = std::unordered_map<Entity, std::vector<Entity>>;

	Entity ResolveUIParent(Registry &registry, Entity entity) const;
	void BuildHierarchy(Registry &registry, std::vector<Entity> &outRoots, ChildrenMap &outChildren) const;
	void ResolveRecursive(Registry &registry,
						  Entity entity,
						  const UIRect &parentRect,
						  const ChildrenMap &children,
						  int &hierarchyCounter) const;
};
#pragma endregion
