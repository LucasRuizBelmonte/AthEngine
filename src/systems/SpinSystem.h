#pragma once
#include "../ecs/Registry.h"

class SpinSystem
{
public:
	void Update(Registry &registry, float timeSec) const;
};