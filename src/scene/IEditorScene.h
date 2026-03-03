/**
 * @file IEditorScene.h
 * @brief Declarations for IEditorScene.
 */

#pragma once

#pragma region Includes
#include <vector>
#include <string>
#include "../ecs/Registry.h"
#include "../ecs/Entity.h"
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
    /**
     * @brief Gets a stable type id used for scene serialization.
     */
    virtual const char *GetEditorSceneType() const = 0;
    /**
     * @brief Saves the editor scene data to disk.
     */
    virtual bool SaveToFile(const std::string &path, const std::string &sceneName, std::string &outError) = 0;
    /**
     * @brief Loads the editor scene data from disk.
     */
    virtual bool LoadFromFile(const std::string &path, std::string &inOutSceneName, std::string &outError) = 0;
    /**
     * @brief Applies a sprite texture path to an entity.
     */
    virtual bool EditorSetSpriteTexture(Entity e, const std::string &path, std::string &outError) = 0;
    /**
     * @brief Applies a sprite material path to an entity.
     */
    virtual bool EditorSetSpriteMaterial(Entity e, const std::string &path, std::string &outError) = 0;
    /**
     * @brief Applies a mesh asset path to an entity.
     */
    virtual bool EditorSetMeshPath(Entity e, const std::string &path, std::string &outError) = 0;
    /**
     * @brief Applies a mesh material path to an entity.
     */
    virtual bool EditorSetMeshMaterial(Entity e, const std::string &path, std::string &outError) = 0;
    /**
     * @brief Applies material texture slots/parameters after editor changes.
     */
    virtual bool EditorApplyMaterial(Entity e, std::string &outError) = 0;
	#pragma endregion
};
#pragma endregion
