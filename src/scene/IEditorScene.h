/**
 * @file IEditorScene.h
 * @brief Declarations for IEditorScene.
 */

#pragma once

#pragma region Includes
#include <vector>
#include "../ecs/Registry.h"
#pragma endregion

#pragma region Declarations
struct EditorSystemToggle
{
    const char *name = "";
    bool *enabled = nullptr;
};

class IEditorScene
{
public:
	#pragma region Public Interface
    /**
     * @brief Executes ~IEditor Scene.
     */
    virtual ~IEditorScene() = default;
    /**
     * @brief Executes Get Editor Registry.
     */
    virtual Registry &GetEditorRegistry() = 0;
    /**
     * @brief Executes Get Editor Systems.
     */
    virtual void GetEditorSystems(std::vector<EditorSystemToggle> &out) = 0;
	#pragma endregion
};
#pragma endregion
