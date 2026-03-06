#pragma region Includes
#include "UILayoutSystem.h"

#include "../../components/Parent.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#pragma endregion

#pragma region File Scope
namespace
{
	static float AlignOffset(float available, float item, UIChildAlignment alignment)
	{
		const float slack = std::max(0.0f, available - item);
		switch (alignment)
		{
		case UIChildAlignment::Center:
			return slack * 0.5f;
		case UIChildAlignment::End:
			return slack;
		case UIChildAlignment::Start:
		default:
			return 0.0f;
		}
	}

	static glm::vec2 ClampNonNegative(const glm::vec2 &v)
	{
		return glm::max(v, glm::vec2(0.0f, 0.0f));
	}
}
#pragma endregion

#pragma region Function Definitions
void UILayoutSystem::Update(Registry &registry, int framebufferWidth, int framebufferHeight)
{
	if (framebufferWidth <= 0 || framebufferHeight <= 0)
		return;

	std::vector<Entity> ordered;
	ChildrenMap children;
	BuildHierarchy(registry, ordered, children);
	if (ordered.empty())
		return;

	RectCache rectCache;
	rectCache.reserve(ordered.size());
	const glm::vec2 screenSize(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));

	for (Entity entity : ordered)
	{
		if (!registry.IsAlive(entity) || !registry.Has<UITransform>(entity))
			continue;

		std::vector<Entity> layoutChildren;
		auto itChildren = children.find(entity);
		if (itChildren != children.end())
			layoutChildren = itChildren->second;

		if (layoutChildren.empty())
			continue;

		std::vector<Entity> recursionGuard;
		const UIRect groupRect = ResolveRectRecursive(registry, entity, screenSize, rectCache, recursionGuard);

		if (registry.Has<UIHorizontalGroup>(entity))
			ApplyHorizontalGroup(registry, entity, registry.Get<UIHorizontalGroup>(entity), layoutChildren, groupRect);
		if (registry.Has<UIVerticalGroup>(entity))
			ApplyVerticalGroup(registry, entity, registry.Get<UIVerticalGroup>(entity), layoutChildren, groupRect);
		if (registry.Has<UIGridGroup>(entity))
			ApplyGridGroup(registry, entity, registry.Get<UIGridGroup>(entity), layoutChildren, groupRect);

		rectCache.clear();
	}
}

UIRect UILayoutSystem::ResolveRectRecursive(Registry &registry,
                                            Entity entity,
                                            const glm::vec2 &screenSize,
                                            RectCache &cache,
                                            std::vector<Entity> &recursionGuard) const
{
	auto it = cache.find(entity);
	if (it != cache.end())
		return it->second;

	if (!registry.IsAlive(entity) || !registry.Has<UITransform>(entity))
	{
		return UIRect{glm::vec2(0.0f, 0.0f), screenSize};
	}

	if (std::find(recursionGuard.begin(), recursionGuard.end(), entity) != recursionGuard.end())
	{
		return UIRect{glm::vec2(0.0f, 0.0f), screenSize};
	}

	recursionGuard.push_back(entity);

	const Entity parent = ResolveUIParent(registry, entity);
	const UIRect parentRect = (parent != kInvalidEntity)
		                        ? ResolveRectRecursive(registry, parent, screenSize, cache, recursionGuard)
		                        : UIRect{glm::vec2(0.0f, 0.0f), screenSize};

	const UITransform &t = registry.Get<UITransform>(entity);
	const glm::vec2 parentSize = parentRect.Size();
	const glm::vec2 anchorMinPx = parentRect.min + t.anchorMin * parentSize;
	const glm::vec2 anchorMaxPx = parentRect.min + t.anchorMax * parentSize;
	const glm::vec2 size = ClampNonNegative((anchorMaxPx - anchorMinPx) + t.sizeDelta);
	const glm::vec2 anchorCenter = (anchorMinPx + anchorMaxPx) * 0.5f;
	const glm::vec2 pivotPos = anchorCenter + t.anchoredPosition;
	const glm::vec2 minPoint = pivotPos - (t.pivot * size);

	const UIRect resolved{minPoint, minPoint + size};
	cache[entity] = resolved;

	recursionGuard.pop_back();
	return resolved;
}

Entity UILayoutSystem::ResolveUIParent(Registry &registry, Entity entity) const
{
	if (!registry.IsAlive(entity) || !registry.Has<Parent>(entity))
		return kInvalidEntity;

	const Entity parent = registry.Get<Parent>(entity).parent;
	if (parent == kInvalidEntity || !registry.IsAlive(parent) || !registry.Has<UITransform>(parent))
		return kInvalidEntity;
	return parent;
}

void UILayoutSystem::BuildHierarchy(Registry &registry, std::vector<Entity> &outOrdered, ChildrenMap &outChildren) const
{
	std::vector<Entity> entities;
	registry.ViewEntities<UITransform>(entities);

	outOrdered.clear();
	outChildren.clear();
	if (entities.empty())
		return;

	std::sort(entities.begin(), entities.end());
	for (Entity entity : entities)
		outChildren[entity] = {};

	std::vector<Entity> roots;
	roots.reserve(entities.size());

	for (Entity entity : entities)
	{
		const Entity parent = ResolveUIParent(registry, entity);
		if (parent == kInvalidEntity)
		{
			roots.push_back(entity);
			continue;
		}
		outChildren[parent].push_back(entity);
	}

	for (auto &kv : outChildren)
		std::sort(kv.second.begin(), kv.second.end());
	std::sort(roots.begin(), roots.end());

	std::unordered_set<Entity> visited;
	visited.reserve(entities.size());

	std::vector<Entity> stack;
	stack.reserve(entities.size());

	for (Entity root : roots)
		stack.push_back(root);

	while (!stack.empty())
	{
		const Entity entity = stack.back();
		stack.pop_back();

		if (!visited.insert(entity).second)
			continue;

		outOrdered.push_back(entity);

		auto itChildren = outChildren.find(entity);
		if (itChildren == outChildren.end())
			continue;

		for (auto it = itChildren->second.rbegin(); it != itChildren->second.rend(); ++it)
			stack.push_back(*it);
	}

	for (Entity entity : entities)
	{
		if (visited.find(entity) == visited.end())
			outOrdered.push_back(entity);
	}
}

void UILayoutSystem::ApplyHorizontalGroup(Registry &registry,
                                          Entity,
                                          const UIHorizontalGroup &group,
                                          const std::vector<Entity> &children,
                                          const UIRect &groupRect)
{
	if (children.empty())
		return;

	const glm::vec2 groupSize = groupRect.Size();
	const glm::vec2 innerMin = groupRect.min + glm::vec2(group.padding.left, group.padding.top);
	const glm::vec2 innerSize = ClampNonNegative(groupSize - glm::vec2(group.padding.left + group.padding.right,
	                                                                    group.padding.top + group.padding.bottom));

	std::vector<LayoutHints> hints;
	hints.reserve(children.size());

	float totalPreferred = 0.0f;
	float totalFlexible = 0.0f;

	for (Entity child : children)
	{
		const LayoutHints h = ResolveLayoutHints(registry, child);
		hints.push_back(h);
		totalPreferred += h.preferredSize.x;

		float weight = std::max(0.0f, h.flexibleSize.x);
		if (group.expandWidth && weight <= 0.0f)
			weight = 1.0f;
		totalFlexible += weight;
	}

	if (children.size() > 1u)
		totalPreferred += group.spacing * static_cast<float>(children.size() - 1u);

	const float extra = innerSize.x - totalPreferred;
	float cursor = innerMin.x;

	for (size_t i = 0; i < children.size(); ++i)
	{
		const LayoutHints &h = hints[i];

		float width = std::max(h.minSize.x, h.preferredSize.x);
		float weight = std::max(0.0f, h.flexibleSize.x);
		if (group.expandWidth && weight <= 0.0f)
			weight = 1.0f;
		if (extra > 0.0f && totalFlexible > 0.0f)
			width += (extra * weight) / totalFlexible;

		float height = std::max(h.minSize.y, h.preferredSize.y);
		if (group.expandHeight)
			height = innerSize.y;

		const float yOffset = AlignOffset(innerSize.y, height, group.childAlignment);
		const glm::vec2 childMin(cursor, innerMin.y + yOffset);
		const glm::vec2 childSize = ClampNonNegative(glm::vec2(width, height));

		ApplyChildRect(registry, children[i], groupRect.min, childMin, childSize);

		cursor += width + group.spacing;
	}
}

void UILayoutSystem::ApplyVerticalGroup(Registry &registry,
                                        Entity,
                                        const UIVerticalGroup &group,
                                        const std::vector<Entity> &children,
                                        const UIRect &groupRect)
{
	if (children.empty())
		return;

	const glm::vec2 groupSize = groupRect.Size();
	const glm::vec2 innerMin = groupRect.min + glm::vec2(group.padding.left, group.padding.top);
	const glm::vec2 innerSize = ClampNonNegative(groupSize - glm::vec2(group.padding.left + group.padding.right,
	                                                                    group.padding.top + group.padding.bottom));

	std::vector<LayoutHints> hints;
	hints.reserve(children.size());

	float totalPreferred = 0.0f;
	float totalFlexible = 0.0f;

	for (Entity child : children)
	{
		const LayoutHints h = ResolveLayoutHints(registry, child);
		hints.push_back(h);
		totalPreferred += h.preferredSize.y;

		float weight = std::max(0.0f, h.flexibleSize.y);
		if (group.expandHeight && weight <= 0.0f)
			weight = 1.0f;
		totalFlexible += weight;
	}

	if (children.size() > 1u)
		totalPreferred += group.spacing * static_cast<float>(children.size() - 1u);

	const float extra = innerSize.y - totalPreferred;
	float cursor = innerMin.y;

	for (size_t i = 0; i < children.size(); ++i)
	{
		const LayoutHints &h = hints[i];

		float height = std::max(h.minSize.y, h.preferredSize.y);
		float weight = std::max(0.0f, h.flexibleSize.y);
		if (group.expandHeight && weight <= 0.0f)
			weight = 1.0f;
		if (extra > 0.0f && totalFlexible > 0.0f)
			height += (extra * weight) / totalFlexible;

		float width = std::max(h.minSize.x, h.preferredSize.x);
		if (group.expandWidth)
			width = innerSize.x;

		const float xOffset = AlignOffset(innerSize.x, width, group.childAlignment);
		const glm::vec2 childMin(innerMin.x + xOffset, cursor);
		const glm::vec2 childSize = ClampNonNegative(glm::vec2(width, height));

		ApplyChildRect(registry, children[i], groupRect.min, childMin, childSize);

		cursor += height + group.spacing;
	}
}

void UILayoutSystem::ApplyGridGroup(Registry &registry,
                                    Entity,
                                    const UIGridGroup &group,
                                    const std::vector<Entity> &children,
                                    const UIRect &groupRect)
{
	if (children.empty())
		return;

	const int count = std::max(1, group.count);
	int columns = 1;
	int rows = 1;

	if (group.constraint == UIGridConstraint::FixedColumns)
	{
		columns = count;
		rows = static_cast<int>(std::ceil(static_cast<float>(children.size()) / static_cast<float>(columns)));
	}
	else
	{
		rows = count;
		columns = static_cast<int>(std::ceil(static_cast<float>(children.size()) / static_cast<float>(rows)));
	}

	const glm::vec2 cell = ClampNonNegative(group.cellSize);
	const glm::vec2 spacing = ClampNonNegative(group.spacing);
	const glm::vec2 groupSize = groupRect.Size();
	const glm::vec2 innerMin = groupRect.min + glm::vec2(group.padding.left, group.padding.top);
	const glm::vec2 innerSize = ClampNonNegative(groupSize - glm::vec2(group.padding.left + group.padding.right,
	                                                                    group.padding.top + group.padding.bottom));

	const glm::vec2 gridSize(
		std::max(0.0f, static_cast<float>(columns) * cell.x + static_cast<float>(columns - 1) * spacing.x),
		std::max(0.0f, static_cast<float>(rows) * cell.y + static_cast<float>(rows - 1) * spacing.y));

	const glm::vec2 gridOrigin(
		innerMin.x + AlignOffset(innerSize.x, gridSize.x, group.alignment),
		innerMin.y + AlignOffset(innerSize.y, gridSize.y, group.alignment));

	for (size_t i = 0; i < children.size(); ++i)
	{
		const int row = static_cast<int>(i) / columns;
		const int column = static_cast<int>(i) % columns;

		const glm::vec2 childMin(
			gridOrigin.x + static_cast<float>(column) * (cell.x + spacing.x),
			gridOrigin.y + static_cast<float>(row) * (cell.y + spacing.y));

		ApplyChildRect(registry, children[i], groupRect.min, childMin, cell);
	}
}

UILayoutSystem::LayoutHints UILayoutSystem::ResolveLayoutHints(Registry &registry, Entity entity) const
{
	LayoutHints hints;

	if (!registry.IsAlive(entity) || !registry.Has<UITransform>(entity))
		return hints;

	const UITransform &transform = registry.Get<UITransform>(entity);
	hints.preferredSize = ClampNonNegative(transform.sizeDelta);

	if (registry.Has<UILayoutElement>(entity))
	{
		const UILayoutElement &layout = registry.Get<UILayoutElement>(entity);
		hints.minSize = ClampNonNegative(layout.minSize);
		if (layout.preferredSize.x > 0.0f)
			hints.preferredSize.x = layout.preferredSize.x;
		if (layout.preferredSize.y > 0.0f)
			hints.preferredSize.y = layout.preferredSize.y;
		hints.flexibleSize = ClampNonNegative(layout.flexibleSize);
	}

	if (registry.Has<UISpacer>(entity))
	{
		const UISpacer &spacer = registry.Get<UISpacer>(entity);
		hints.preferredSize = ClampNonNegative(spacer.preferredSize);
		hints.flexibleSize = ClampNonNegative(spacer.flexibleSize);
		hints.minSize = glm::max(hints.minSize, glm::vec2(0.0f, 0.0f));
	}

	hints.preferredSize = glm::max(hints.preferredSize, hints.minSize);
	return hints;
}

void UILayoutSystem::ApplyChildRect(Registry &registry,
                                    Entity childEntity,
                                    const glm::vec2 &parentMin,
                                    const glm::vec2 &childMin,
                                    const glm::vec2 &childSize)
{
	if (!registry.IsAlive(childEntity) || !registry.Has<UITransform>(childEntity))
		return;

	UITransform &child = registry.Get<UITransform>(childEntity);
	child.anchorMin = glm::vec2(0.0f, 0.0f);
	child.anchorMax = glm::vec2(0.0f, 0.0f);
	child.sizeDelta = ClampNonNegative(childSize);

	const glm::vec2 localMin = childMin - parentMin;
	child.anchoredPosition = localMin + child.pivot * child.sizeDelta;
}
#pragma endregion
