#pragma region Includes
#include "SpinSystem.h"
#include "../components/Transform.h"
#include "../components/Spin.h"
#include <glm/glm.hpp>
#include <cmath>
#include <vector>
#pragma endregion

#pragma region Function Definitions
void SpinSystem::Update(Registry &registry, float timeSec) const
{
	std::vector<Entity> entities;
	registry.ViewEntities<Transform, Spin>(entities);

	for (Entity e : entities)
	{
		auto &t = registry.Get<Transform>(e);
		const auto &s = registry.Get<Spin>(e);

		float angle = std::sin(timeSec * s.freq) * s.amplitude;

		if (s.axis.x == 0.f && s.axis.y == 0.f && s.axis.z == 1.f)
		{
			t.rotationEuler.z = angle;
		}
		else if (s.axis.x == 0.f && s.axis.y == 0.f && s.axis.z == -1.f)
		{
			t.rotationEuler.z = -angle;
		}
		else
		{
			t.rotationEuler = s.axis * angle;
		}
	}
}
#pragma endregion
