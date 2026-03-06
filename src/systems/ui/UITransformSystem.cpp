#pragma region Includes
#include "UITransformSystem.h"

#include "../../components/Parent.h"
#include "../../components/Transform.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#pragma endregion

#pragma region File Scope
namespace
{
	static glm::vec2 ClampNonNegative(const glm::vec2 &v)
	{
		return glm::max(v, glm::vec2(0.0f, 0.0f));
	}

	static glm::mat4 BuildUIWorldMatrix(const UITransform &t, const glm::vec2 &size, const glm::vec2 &pivotPos)
	{
		const glm::vec2 scaledSize = size * t.scale;
		const glm::vec2 pivotUnit = t.pivot - glm::vec2(0.5f, 0.5f);

		const glm::mat4 translateToPivot = glm::translate(glm::mat4(1.0f), glm::vec3(pivotPos, 0.0f));
		const glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), t.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		const glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scaledSize, 1.0f));
		const glm::mat4 pivotShift = glm::translate(glm::mat4(1.0f), glm::vec3(-pivotUnit, 0.0f));

		return translateToPivot * rotate * scale * pivotShift;
	}
}
#pragma endregion

#pragma region Function Definitions
void UITransformSystem::Update(Registry &registry, int framebufferWidth, int framebufferHeight)
{
	if (framebufferWidth <= 0 || framebufferHeight <= 0)
		return;

	std::vector<Entity> entities;
	registry.ViewEntities<UITransform>(entities);
	if (entities.empty())
		return;

	for (Entity entity : entities)
	{
		if (registry.Has<Transform>(entity))
			registry.Remove<Transform>(entity);
	}

	for (Entity entity : entities)
		registry.Get<UITransform>(entity).hierarchyIndex = -1;

	std::vector<Entity> roots;
	ChildrenMap children;
	BuildHierarchy(registry, roots, children);

	const UIRect screenRect{glm::vec2(0.0f, 0.0f), glm::vec2(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight))};
	int hierarchyCounter = 0;
	for (Entity root : roots)
		ResolveRecursive(registry, root, screenRect, children, hierarchyCounter);

	for (Entity entity : entities)
	{
		UITransform &t = registry.Get<UITransform>(entity);
		if (t.hierarchyIndex >= 0)
			continue;
		ResolveRecursive(registry, entity, screenRect, children, hierarchyCounter);
	}
}

Entity UITransformSystem::ResolveUIParent(Registry &registry, Entity entity) const
{
	if (!registry.IsAlive(entity) || !registry.Has<Parent>(entity))
		return kInvalidEntity;

	const Entity parent = registry.Get<Parent>(entity).parent;
	if (parent == kInvalidEntity || !registry.IsAlive(parent) || !registry.Has<UITransform>(parent))
		return kInvalidEntity;

	return parent;
}

void UITransformSystem::BuildHierarchy(Registry &registry, std::vector<Entity> &outRoots, ChildrenMap &outChildren) const
{
	std::vector<Entity> entities;
	registry.ViewEntities<UITransform>(entities);

	outRoots.clear();
	outChildren.clear();
	if (entities.empty())
		return;

	std::sort(entities.begin(), entities.end());
	for (Entity entity : entities)
		outChildren[entity] = {};

	for (Entity entity : entities)
	{
		const Entity parent = ResolveUIParent(registry, entity);
		if (parent == kInvalidEntity)
		{
			outRoots.push_back(entity);
			continue;
		}
		outChildren[parent].push_back(entity);
	}

	for (auto &kv : outChildren)
		std::sort(kv.second.begin(), kv.second.end());
	std::sort(outRoots.begin(), outRoots.end());
}

void UITransformSystem::ResolveRecursive(Registry &registry,
                                         Entity entity,
                                         const UIRect &parentRect,
                                         const ChildrenMap &children,
                                         int &hierarchyCounter) const
{
	if (!registry.IsAlive(entity) || !registry.Has<UITransform>(entity))
		return;

	UITransform &t = registry.Get<UITransform>(entity);
	if (t.hierarchyIndex >= 0)
		return;

	const glm::vec2 parentSize = parentRect.Size();
	const glm::vec2 anchorMinPx = parentRect.min + t.anchorMin * parentSize;
	const glm::vec2 anchorMaxPx = parentRect.min + t.anchorMax * parentSize;
	const glm::vec2 size = ClampNonNegative((anchorMaxPx - anchorMinPx) + t.sizeDelta);
	const glm::vec2 anchorCenter = (anchorMinPx + anchorMaxPx) * 0.5f;
	const glm::vec2 pivotPos = anchorCenter + t.anchoredPosition;
	const glm::vec2 rectMin = pivotPos - (t.pivot * size);
	const glm::vec2 rectMax = rectMin + size;

	t.worldRect.min = rectMin;
	t.worldRect.max = rectMax;
	t.worldMatrix = BuildUIWorldMatrix(t, size, pivotPos);
	t.hierarchyIndex = hierarchyCounter++;

	auto it = children.find(entity);
	if (it == children.end())
		return;

	for (Entity child : it->second)
		ResolveRecursive(registry, child, t.worldRect, children, hierarchyCounter);
}
#pragma endregion
