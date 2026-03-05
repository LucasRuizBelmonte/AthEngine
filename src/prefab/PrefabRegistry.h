/**
 * @file PrefabRegistry.h
 * @brief Prefab registry and spawn API declarations.
 */

#pragma once

#pragma region Includes
#include "Prefab.h"

#include "../components/Transform.h"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region Declarations
struct Sprite;

namespace prefab
{
	struct PrefabSpawnOverrides
	{
		std::optional<Transform> transform;
		std::optional<glm::vec4> spriteTint;
		std::optional<glm::vec2> spriteSize;
	};

	class PrefabRegistry
	{
	public:
		Prefab &Register(const std::string &name);
		Prefab &Register(const std::string &name, Prefab prefab);
		void Clear();

		bool Has(const std::string &name) const;
		const Prefab *Find(const std::string &name) const;
		Prefab *Find(const std::string &name);

		void GetNames(std::vector<std::string> &outNames) const;

		Entity SpawnPrefab(Registry &registry, const std::string &name, const Transform &at) const;
		Entity SpawnPrefab(Registry &registry,
		                   const std::string &name,
		                   const Transform &at,
		                   const PrefabSpawnOverrides &overrides) const;

	private:
		static void ApplyOverrides(Registry &registry,
		                           Entity entity,
		                           const Transform &at,
		                           const PrefabSpawnOverrides &overrides);

		std::unordered_map<std::string, Prefab> m_prefabs;
	};
}
#pragma endregion
