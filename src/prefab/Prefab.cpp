#pragma region Includes
#include "Prefab.h"
#pragma endregion

#pragma region Function Definitions
namespace prefab
{
	Prefab &Prefab::AddInstaller(ComponentInstaller installer)
	{
		m_installers.emplace_back(std::move(installer));
		return *this;
	}

	void Prefab::Instantiate(Registry &registry, Entity entity) const
	{
		for (const ComponentInstaller &installer : m_installers)
			installer(registry, entity);
	}
}
#pragma endregion
