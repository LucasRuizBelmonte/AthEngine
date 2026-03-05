#pragma region Includes
#include "BuiltinPrefabs.h"

#include "../components/Tag.h"
#include "../components/Sprite.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/RigidBody2D.h"
#pragma endregion

#pragma region Function Definitions
namespace prefab
{
	namespace
	{
		static Prefab BuildPlayerPrefab()
		{
			Prefab prefab;

			Sprite sprite;
			sprite.size = glm::vec2(1.0f, 1.2f);
			sprite.tint = glm::vec4(0.35f, 0.85f, 0.45f, 1.0f);
			sprite.texturePath = "res/textures/sprite.png";
			sprite.materialPath = "res/shaders/sprite.fs";

			Collider2D collider;
			collider.shape = Collider2D::Shape::AABB;
			collider.halfExtents = glm::vec2(0.45f, 0.55f);

			RigidBody2D rigidBody;
			rigidBody.mass = 1.0f;
			rigidBody.linearDamping = 1.0f;

			prefab
				.AddComponent(Tag{"Player"})
				.AddComponent(Transform{})
				.AddComponent(sprite)
				.AddComponent(collider)
				.AddComponent(rigidBody);

			return prefab;
		}

		static Prefab BuildEnemyBasicPrefab()
		{
			Prefab prefab;

			Sprite sprite;
			sprite.size = glm::vec2(0.9f, 0.9f);
			sprite.tint = glm::vec4(0.95f, 0.30f, 0.30f, 1.0f);
			sprite.texturePath = "res/textures/sprite_2.png";
			sprite.materialPath = "res/shaders/sprite.fs";

			Collider2D collider;
			collider.shape = Collider2D::Shape::Circle;
			collider.radius = 0.45f;

			RigidBody2D rigidBody;
			rigidBody.mass = 0.7f;
			rigidBody.linearDamping = 0.5f;

			prefab
				.AddComponent(Tag{"EnemyBasic"})
				.AddComponent(Transform{})
				.AddComponent(sprite)
				.AddComponent(collider)
				.AddComponent(rigidBody);

			return prefab;
		}
	}

	void RegisterBuiltinPrefabs(PrefabRegistry &registry)
	{
		registry.Register("Player", BuildPlayerPrefab());
		registry.Register("EnemyBasic", BuildEnemyBasicPrefab());
	}
}
#pragma endregion
