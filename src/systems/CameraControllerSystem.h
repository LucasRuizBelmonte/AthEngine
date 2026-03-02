#pragma once
#include "../ecs/Registry.h"

struct GLFWwindow;

class CameraControllerSystem
{
public:
	void Update(Registry &registry, GLFWwindow *window, Entity cameraEntity, float dt) const;
};