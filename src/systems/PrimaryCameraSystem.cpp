#pragma region Includes
#include "PrimaryCameraSystem.h"

#include "../components/Camera.h"
#include "../components/runtime/RuntimeResources.h"

#include <vector>
#pragma endregion

#pragma region Function Definitions
void PrimaryCameraSystem::Reset(Registry &registry) const
{
	registry.EnsureResource<RuntimePrimaryCamera>().entity = kInvalidEntity;
}

Entity PrimaryCameraSystem::Resolve(Registry &registry) const
{
	RuntimePrimaryCamera &state = registry.EnsureResource<RuntimePrimaryCamera>();
	if (state.entity != kInvalidEntity &&
	    registry.IsAlive(state.entity) &&
	    registry.Has<Camera>(state.entity) &&
	    registry.IsComponentEnabled<Camera>(state.entity))
	{
		return state.entity;
	}

	std::vector<Entity> cameras;
	registry.ViewEntities<Camera>(cameras);
	state.entity = cameras.empty() ? kInvalidEntity : cameras.front();
	return state.entity;
}
#pragma endregion
