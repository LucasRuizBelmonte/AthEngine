/**
 * @file UILayoutSystem.h
 * @brief Declarations for UILayoutSystem.
 */

#pragma once

#pragma region Includes
#include "../../ecs/Registry.h"
#include "../../components/ui/UIComponents.h"

#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region Declarations
class UILayoutSystem
{
public:
	void Update(Registry &registry, int framebufferWidth, int framebufferHeight);

private:
	struct LayoutHints
	{
		glm::vec2 minSize{0.0f, 0.0f};
		glm::vec2 preferredSize{0.0f, 0.0f};
		glm::vec2 flexibleSize{0.0f, 0.0f};
	};

	using ChildrenMap = std::unordered_map<Entity, std::vector<Entity>>;
	using RectCache = std::unordered_map<Entity, UIRect>;

	UIRect ResolveRectRecursive(Registry &registry,
	                           Entity entity,
	                           const glm::vec2 &screenSize,
	                           RectCache &cache,
	                           std::vector<Entity> &recursionGuard) const;

	Entity ResolveUIParent(Registry &registry, Entity entity) const;
	void BuildHierarchy(Registry &registry, std::vector<Entity> &outOrdered, ChildrenMap &outChildren) const;

	void ApplyHorizontalGroup(Registry &registry,
	                          Entity groupEntity,
	                          const UIHorizontalGroup &group,
	                          const std::vector<Entity> &children,
	                          const UIRect &groupRect);

	void ApplyVerticalGroup(Registry &registry,
	                        Entity groupEntity,
	                        const UIVerticalGroup &group,
	                        const std::vector<Entity> &children,
	                        const UIRect &groupRect);

	void ApplyGridGroup(Registry &registry,
	                    Entity groupEntity,
	                    const UIGridGroup &group,
	                    const std::vector<Entity> &children,
	                    const UIRect &groupRect);

	LayoutHints ResolveLayoutHints(Registry &registry, Entity entity) const;
	void ApplyChildRect(Registry &registry, Entity childEntity, const glm::vec2 &parentMin, const glm::vec2 &childMin, const glm::vec2 &childSize);
};
#pragma endregion
