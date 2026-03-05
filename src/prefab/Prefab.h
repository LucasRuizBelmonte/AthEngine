/**
 * @file Prefab.h
 * @brief Type-safe prefab template declarations.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"

#include <functional>
#include <utility>
#include <vector>
#pragma endregion

#pragma region Declarations
namespace prefab
{
	class Prefab
	{
	public:
		using ComponentInstaller = std::function<void(Registry &, Entity)>;

		Prefab &AddInstaller(ComponentInstaller installer);

		template <typename T>
		Prefab &AddComponent(const T &component)
		{
			m_installers.emplace_back([component](Registry &registry, Entity entity)
									  { registry.Emplace<T>(entity, component); });
			return *this;
		}

		void Instantiate(Registry &registry, Entity entity) const;

	private:
		std::vector<ComponentInstaller> m_installers;
	};
}
#pragma endregion
