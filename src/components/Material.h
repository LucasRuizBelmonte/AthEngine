#pragma once

#include "../resources/ResourceHandle.h"

class Shader;

struct Material
{
    ResourceHandle<Shader> shader;
};