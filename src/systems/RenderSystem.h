#pragma once
#include "../ecs/Registry.h"

class Renderer;

class RenderSystem
{
public:
    void Render(Registry &registry,
                Renderer &renderer,
                Entity cameraEntity,
                int framebufferWidth,
                int framebufferHeight) const;
};