#pragma once
#include "../ecs/Registry.h"

struct GLFWwindow;
class Renderer;

class RenderSystem
{
public:
    void Render(Registry &registry,
                Renderer &renderer,
                GLFWwindow *window) const;
};