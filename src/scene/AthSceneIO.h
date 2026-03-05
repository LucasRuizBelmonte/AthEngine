/**
 * @file AthSceneIO.h
 * @brief Structured scene reader/writer internals for .athscene files.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"

#include <string>
#include <unordered_map>
#pragma endregion

#pragma region Declarations
namespace AthSceneIO
{
	struct SceneHeader
	{
		std::string sceneType;
		std::string sceneName;
	};

	class ScenePathResolver
	{
	public:
		static std::string ToRelativePathForSave(const std::string &rawPath);
	};

	class EntityRemapper
	{
	public:
		void Reserve(size_t count);
		void Map(Entity source, Entity target);
		Entity Resolve(Entity source) const;

	private:
		std::unordered_map<Entity, Entity> m_remap;
	};

	struct ComponentCodecs
	{
		struct TagCodec;
		struct ParentCodec;
		struct TransformCodec;
		struct CameraCodec;
		struct CameraControllerCodec;
		struct SpinCodec;
		struct LightEmitterCodec;
		struct SpriteCodec;
		struct SpriteAnimatorCodec;
		struct Collider2DCodec;
		struct RigidBody2DCodec;
		struct PhysicsBody2DCodec;
		struct MaterialCodec;
		struct MeshCodec;
	};

	class AthSceneWriter
	{
	public:
		static bool SaveRegistry(const Registry &registry,
		                         const std::string &sceneType,
		                         const std::string &sceneName,
		                         const std::string &path,
		                         std::string &outError);
	};

	class AthSceneReader
	{
	public:
		static bool PeekHeader(const std::string &path, SceneHeader &outHeader, std::string &outError);
		static bool LoadRegistry(Registry &registry,
		                         const std::string &expectedSceneType,
		                         std::string &inOutSceneName,
		                         const std::string &path,
		                         std::string &outError);
	};
}
#pragma endregion
