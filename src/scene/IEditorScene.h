#pragma once
#include <vector>
#include "../ecs/Registry.h"

struct EditorSystemToggle
{
    const char* name = "";
    bool* enabled = nullptr;
};

class IEditorScene
{
public:
    virtual ~IEditorScene() = default;

    virtual Registry& GetEditorRegistry() = 0;
    virtual void GetEditorSystems(std::vector<EditorSystemToggle>& out) = 0;
};