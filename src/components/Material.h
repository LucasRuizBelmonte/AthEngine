#pragma once
#include <glm/glm.hpp>
#include "../resources/ResourceHandle.h"

class Shader;
class Texture;

struct Material
{
    ResourceHandle<Shader> shader;
    ResourceHandle<Texture> texture;
    glm::vec4 tint{1.f, 1.f, 1.f, 1.f};
};