#pragma once
#include "../ecs/Entity.h"

class Registry;

struct WindowContext
{
	Registry *registry = nullptr;
	Entity cameraEntity = kInvalidEntity;
};