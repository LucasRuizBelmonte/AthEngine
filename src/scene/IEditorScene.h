/**
 * @file IEditorScene.h
 * @brief Declarations for IEditorScene.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../ecs/Registry.h"
#include "../ecs/Entity.h"
#pragma endregion

#pragma region Declarations
struct EditorSystemToggle
{
    const char *name = "";
    bool *enabled = nullptr;
};

enum class EditorSceneDimension : uint8_t
{
    Scene3D = 0,
    Scene2D = 1
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
     * @brief Returns whether scene editing/runtime is 2D or 3D constrained.
     */
    virtual EditorSceneDimension GetEditorSceneDimension() const = 0;
    /**
     * @brief Sets the scene dimension and applies compatibility rules.
     */
    virtual void SetEditorSceneDimension(EditorSceneDimension dimension) = 0;
    /**
     * @brief Enables/disables editor camera input for this scene.
     */
    virtual void SetEditorInputEnabled(bool enabled) = 0;
    /**
     * @brief Sets global 2D physics gravity for this scene.
     */
    virtual void SetPhysics2DGravity(const glm::vec2 &gravity) = 0;
    /**
     * @brief Gets global 2D physics gravity for this scene.
     */
    virtual glm::vec2 GetPhysics2DGravity() const = 0;
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
