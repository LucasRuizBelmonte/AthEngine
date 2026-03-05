#pragma region Includes
#include "PrefabRegistry.h"

#include "../components/Sprite.h"

#include <algorithm>
#pragma endregion

#pragma region Function Definitions
namespace prefab
{
	Prefab &PrefabRegistry::Register(const std::string &name)
	{
		return m_prefabs[name];
	}

	Prefab &PrefabRegistry::Register(const std::string &name, Prefab prefab)
	{
		auto [it, _] = m_prefabs.insert_or_assign(name, std::move(prefab));
		return it->second;
	}

	void PrefabRegistry::Clear()
	{
		m_prefabs.clear();
	}

	bool PrefabRegistry::Has(const std::string &name) const
	{
		return m_prefabs.find(name) != m_prefabs.end();
	}

	const Prefab *PrefabRegistry::Find(const std::string &name) const
	{
		const auto it = m_prefabs.find(name);
		if (it == m_prefabs.end())
			return nullptr;
		return &it->second;
	}

	Prefab *PrefabRegistry::Find(const std::string &name)
	{
		const auto it = m_prefabs.find(name);
		if (it == m_prefabs.end())
			return nullptr;
		return &it->second;
	}

	void PrefabRegistry::GetNames(std::vector<std::string> &outNames) const
	{
		outNames.clear();
		outNames.reserve(m_prefabs.size());
		for (const auto &entry : m_prefabs)
			outNames.push_back(entry.first);
		std::sort(outNames.begin(), outNames.end());
	}

	Entity PrefabRegistry::SpawnPrefab(Registry &registry, const std::string &name, const Transform &at) const
	{
		return SpawnPrefab(registry, name, at, PrefabSpawnOverrides{});
	}

	Entity PrefabRegistry::SpawnPrefab(Registry &registry,
	                                   const std::string &name,
	                                   const Transform &at,
	                                   const PrefabSpawnOverrides &overrides) const
	{
		const Prefab *prefab = Find(name);
		if (!prefab)
			return kInvalidEntity;

		const Entity entity = registry.Create();
		if (entity == kInvalidEntity)
			return kInvalidEntity;

		prefab->Instantiate(registry, entity);
		ApplyOverrides(registry, entity, at, overrides);

		return entity;
	}

	void PrefabRegistry::ApplyOverrides(Registry &registry,
	                                    Entity entity,
	                                    const Transform &at,
	                                    const PrefabSpawnOverrides &overrides)
	{
		const Transform transformToApply = overrides.transform.has_value() ? overrides.transform.value() : at;
		registry.Emplace<Transform>(entity, transformToApply);

		if (registry.Has<Sprite>(entity))
		{
			Sprite &sprite = registry.Get<Sprite>(entity);
			if (overrides.spriteTint.has_value())
				sprite.tint = overrides.spriteTint.value();
			if (overrides.spriteSize.has_value())
				sprite.size = overrides.spriteSize.value();
		}
	}
}
#pragma endregion
