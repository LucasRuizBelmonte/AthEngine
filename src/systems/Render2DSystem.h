#pragma once
#include "../ecs/Registry.h"

class Renderer;

class Render2DSystem
{
public:
	void Render(Registry &registry,
				Renderer &renderer,
				Entity cameraEntity,
				int framebufferWidth,
				int framebufferHeight,
				const struct Mesh &quadMesh) const;
};