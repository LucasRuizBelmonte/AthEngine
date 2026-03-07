#pragma once

#include "AthSceneIO.h"

#include "../animation2d/SpriteAnimator.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/LightEmitter.h"
#include "../components/Material.h"
#include "../components/Mesh.h"
#include "../components/Parent.h"
#include "../components/Sprite.h"
#include "../components/Spin.h"
#include "../components/Tag.h"
#include "../components/Transform.h"
#include "../components/ui/UIComponents.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/PhysicsBody2D.h"
#include "../physics2d/RigidBody2D.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace AthSceneIO::internal
{
	struct SavedEntity
	{
		Entity id = kInvalidEntity;
		bool hasTag = false;
		Tag tag;

		bool hasParent = false;
		Entity parent = kInvalidEntity;

		bool hasTransform = false;
		Transform transform;

		bool hasCamera = false;
		Camera camera;

		bool hasCameraController = false;
		CameraController cameraController;

		bool hasSpin = false;
		Spin spin;

		bool hasLight = false;
		LightEmitter light;

		bool hasSprite = false;
		Sprite sprite;
		bool hasUITransform = false;
		UITransform uiTransform;
		bool hasUISprite = false;
		UISprite uiSprite;
		bool hasUIText = false;
		UIText uiText;
		bool hasUIHorizontalGroup = false;
		UIHorizontalGroup uiHorizontalGroup;
		bool hasUIVerticalGroup = false;
		UIVerticalGroup uiVerticalGroup;
		bool hasUIGridGroup = false;
		UIGridGroup uiGridGroup;
		bool hasUILayoutElement = false;
		UILayoutElement uiLayoutElement;
		bool hasUISpacer = false;
		UISpacer uiSpacer;
		bool hasUIMask = false;
		UIMask uiMask;
		bool hasUIFill = false;
		UIFill uiFill;
		bool hasHealth = false;
		Health health;
		bool hasSpriteAnimator = false;
		SpriteAnimator spriteAnimator;

		bool hasCollider2D = false;
		Collider2D collider2D;

		bool hasRigidBody2D = false;
		RigidBody2D rigidBody2D;

		bool hasPhysicsBody2D = false;
		PhysicsBody2D physicsBody2D;

		bool hasMaterial = false;
		Material material;

		bool hasMesh = false;
		std::string meshPath;
		std::string meshMaterialPath;
	};

	SpritePivot SpritePivotFromStoredValue(int value);
	bool IsValidSpritePivotValue(int value);
	UITextAlignment UITextAlignmentFromStoredValue(int value);
	bool IsValidUITextAlignmentValue(int value);
	UIChildAlignment UIChildAlignmentFromStoredValue(int value);
	bool IsValidUIChildAlignmentValue(int value);
	UIGridConstraint UIGridConstraintFromStoredValue(int value);
	bool IsValidUIGridConstraintValue(int value);
	UIFillDirection UIFillDirectionFromStoredValue(int value);
	bool IsValidUIFillDirectionValue(int value);

	bool ReadExpectedKeyword(std::istream &in, const char *expected, std::string &outError);
	std::string ReadLinePayload(std::istream &in);
	std::string BuildSchemaError(const std::string &componentName,
	                            const std::string &expectedSchema,
	                            const std::string &actualData);
	bool ParseFloatPayload(const std::string &payload,
	                      const std::string &context,
	                      std::vector<float> &outValues,
	                      std::string &outError);
	void ClearRegistry(Registry &registry);
}

namespace AthSceneIO
{
	struct ComponentCodecs::TagCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::ParentCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::TransformCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::CameraCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::CameraControllerCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::SpinCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::LightEmitterCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::SpriteCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UITransformCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UISpriteCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UITextCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UIHorizontalGroupCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UIVerticalGroupCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UIGridGroupCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UILayoutElementCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UISpacerCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UIMaskCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::UIFillCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::HealthCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::SpriteAnimatorCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::Collider2DCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::RigidBody2DCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::PhysicsBody2DCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::MaterialCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool ReadV2(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};

	struct ComponentCodecs::MeshCodec
	{
		static void Write(const Registry &registry, Entity entity, std::ostream &out);
		static bool Read(std::istream &in, internal::SavedEntity &ent, std::string &outError);
	};
}
