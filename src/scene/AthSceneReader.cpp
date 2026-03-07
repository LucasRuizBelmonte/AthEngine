#include "AthSceneIOInternal.h"

#include "../utils/StrictParsing.h"

#include <functional>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

namespace AthSceneIO
{
	bool AthSceneReader::PeekHeader(const std::string &path, SceneHeader &outHeader, std::string &outError)
	{
		bool success = false;
		auto errorPathScope = [&]()
		{
			if (!success && !outError.empty() && outError.rfind(path + ":", 0) != 0)
				outError = path + ": " + outError;
		};
		struct ScopeExit
		{
			std::function<void()> fn;
			~ScopeExit() { fn(); }
		} scope{errorPathScope};

		std::ifstream in(path);
		if (!in)
		{
			outError = "Could not open file: " + path;
			return false;
		}

		std::string magic;
		int version = 0;
		if (!(in >> magic >> version) || magic != "ATHSCENE" || version != 1)
		{
			outError = "Not a supported .athscene file (expected ATHSCENE 1).";
			return false;
		}
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. Expected 'ATHSCENE 1' on its own line.";
			return false;
		}

		if (!internal::ReadExpectedKeyword(in, "TYPE", outError))
			return false;
		if (!(in >> outHeader.sceneType))
		{
			outError = "Failed reading scene type.";
			return false;
		}
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. TYPE line has trailing tokens.";
			return false;
		}

		if (!internal::ReadExpectedKeyword(in, "NAME", outError))
			return false;
		if (!(in >> std::quoted(outHeader.sceneName)))
		{
			outError = "Failed reading scene name.";
			return false;
		}
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. NAME line has trailing tokens.";
			return false;
		}

		success = true;
		return true;
	}

	bool AthSceneReader::LoadRegistry(Registry &registry,
	                                  const std::string &expectedSceneType,
	                                  std::string &inOutSceneName,
	                                  const std::string &path,
	                                  std::string &outError)
	{
		bool success = false;
		auto errorPathScope = [&]()
		{
			if (!success && !outError.empty() && outError.rfind(path + ":", 0) != 0)
				outError = path + ": " + outError;
		};
		struct ScopeExit
		{
			std::function<void()> fn;
			~ScopeExit() { fn(); }
		} scope{errorPathScope};

		std::ifstream in(path);
		if (!in)
		{
			outError = "Could not open file: " + path;
			return false;
		}

		std::string magic;
		int version = 0;
		if (!(in >> magic >> version) || magic != "ATHSCENE" || version != 1)
		{
			outError = "Not a supported .athscene file (expected ATHSCENE 1).";
			return false;
		}
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. Expected 'ATHSCENE 1' on its own line.";
			return false;
		}

		if (!internal::ReadExpectedKeyword(in, "TYPE", outError))
			return false;

		std::string sceneType;
		if (!(in >> sceneType))
		{
			outError = "Failed reading scene type.";
			return false;
		}
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. TYPE line has trailing tokens.";
			return false;
		}

		if (sceneType != expectedSceneType)
		{
			outError = "Scene type mismatch. File type is '" + sceneType + "', expected '" + expectedSceneType + "'.";
			return false;
		}

		if (!internal::ReadExpectedKeyword(in, "NAME", outError))
			return false;
		if (!(in >> std::quoted(inOutSceneName)))
		{
			outError = "Failed reading scene name.";
			return false;
		}
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. NAME line has trailing tokens.";
			return false;
		}

		if (!internal::ReadExpectedKeyword(in, "ENTITY_COUNT", outError))
			return false;

		size_t count = 0;
		if (!(in >> count))
		{
			outError = "Failed reading entity count.";
			return false;
		}
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene schema mismatch. ENTITY_COUNT line has trailing tokens.";
			return false;
		}

		std::vector<internal::SavedEntity> saved;
		saved.reserve(count);

		for (size_t i = 0; i < count; ++i)
		{
			if (!internal::ReadExpectedKeyword(in, "ENTITY", outError))
				return false;

			internal::SavedEntity ent;
			if (!(in >> ent.id))
			{
				outError = "Failed reading entity id.";
				return false;
			}
			if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
			{
				outError = "Scene schema mismatch. ENTITY line has trailing tokens.";
				return false;
			}

			while (true)
			{
				std::string key;
				if (!(in >> key))
				{
					outError = "Unexpected end of file while reading entity data.";
					return false;
				}

				if (key == "END_ENTITY")
					break;

				if (key == "TAG")
				{
					if (!ComponentCodecs::TagCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "PARENT")
				{
					if (!ComponentCodecs::ParentCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "TRANSFORM")
				{
					if (!ComponentCodecs::TransformCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "CAMERA")
				{
					if (!ComponentCodecs::CameraCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "CAMERA_CONTROLLER")
				{
					if (!ComponentCodecs::CameraControllerCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "SPIN")
				{
					if (!ComponentCodecs::SpinCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "LIGHT")
				{
					if (!ComponentCodecs::LightEmitterCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "SPRITE")
				{
					if (!ComponentCodecs::SpriteCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UITRANSFORM")
				{
					if (!ComponentCodecs::UITransformCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UISPRITE")
				{
					if (!ComponentCodecs::UISpriteCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UITEXT")
				{
					if (!ComponentCodecs::UITextCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIHORIZONTALGROUP")
				{
					if (!ComponentCodecs::UIHorizontalGroupCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIVERTICALGROUP")
				{
					if (!ComponentCodecs::UIVerticalGroupCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIGRIDGROUP")
				{
					if (!ComponentCodecs::UIGridGroupCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UILAYOUTELEMENT")
				{
					if (!ComponentCodecs::UILayoutElementCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UISPACER")
				{
					if (!ComponentCodecs::UISpacerCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIMASK")
				{
					if (!ComponentCodecs::UIMaskCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIFILL")
				{
					if (!ComponentCodecs::UIFillCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "HEALTH")
				{
					if (!ComponentCodecs::HealthCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "SPRITE_ANIMATOR")
				{
					if (!ComponentCodecs::SpriteAnimatorCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "COLLIDER2D")
				{
					if (!ComponentCodecs::Collider2DCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "RIGIDBODY2D")
				{
					if (!ComponentCodecs::RigidBody2DCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "PHYSICSBODY2D")
				{
					if (!ComponentCodecs::PhysicsBody2DCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "MATERIAL_V2")
				{
					if (!ComponentCodecs::MaterialCodec::ReadV2(in, ent, outError))
						return false;
					continue;
				}

				if (key == "MESH")
				{
					if (!ComponentCodecs::MeshCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				outError = "Unknown component token '" + key +
				           "' in scene file. Expected one of TAG,PARENT,TRANSFORM,CAMERA,CAMERA_CONTROLLER,SPIN,LIGHT,SPRITE,UITRANSFORM,UISPRITE,UITEXT,UIHORIZONTALGROUP,UIVERTICALGROUP,UIGRIDGROUP,UILAYOUTELEMENT,UISPACER,UIMASK,UIFILL,HEALTH,SPRITE_ANIMATOR,COLLIDER2D,RIGIDBODY2D,PHYSICSBODY2D,MATERIAL_V2,MESH,END_ENTITY.";
				return false;
			}

			saved.push_back(std::move(ent));
		}

		if (!internal::ReadExpectedKeyword(in, "END_SCENE", outError))
			return false;
		if (!StrictParsing::TrimCopy(internal::ReadLinePayload(in)).empty())
		{
			outError = "Scene schema mismatch. END_SCENE line has trailing tokens.";
			return false;
		}
		std::string trailingToken;
		if (in >> trailingToken)
		{
			outError = "Scene schema mismatch. Unexpected trailing token after END_SCENE: '" + trailingToken + "'.";
			return false;
		}

		internal::ClearRegistry(registry);

		EntityRemapper remapper;
		remapper.Reserve(saved.size());
		for (const internal::SavedEntity &ent : saved)
		{
			const Entity newEntity = registry.Create();
			remapper.Map(ent.id, newEntity);
		}

		for (const internal::SavedEntity &ent : saved)
		{
			const Entity entity = remapper.Resolve(ent.id);
			if (entity == kInvalidEntity || !registry.IsAlive(entity))
				continue;

			if (ent.hasTag)
				registry.Emplace<Tag>(entity, ent.tag);

			if (ent.hasParent)
			{
				Parent p;
				p.parent = remapper.Resolve(ent.parent);
				registry.Emplace<Parent>(entity, p);
			}

			if (ent.hasTransform && !ent.hasUITransform)
				registry.Emplace<Transform>(entity, ent.transform);
			if (ent.hasCamera)
				registry.Emplace<Camera>(entity, ent.camera);
			if (ent.hasCameraController)
				registry.Emplace<CameraController>(entity, ent.cameraController);
			if (ent.hasSpin)
				registry.Emplace<Spin>(entity, ent.spin);
			if (ent.hasLight)
				registry.Emplace<LightEmitter>(entity, ent.light);
			if (ent.hasSprite)
				registry.Emplace<Sprite>(entity, ent.sprite);
			if (ent.hasUITransform)
				registry.Emplace<UITransform>(entity, ent.uiTransform);
			if (ent.hasUISprite)
				registry.Emplace<UISprite>(entity, ent.uiSprite);
			if (ent.hasUIText)
				registry.Emplace<UIText>(entity, ent.uiText);
			if (ent.hasUIHorizontalGroup)
				registry.Emplace<UIHorizontalGroup>(entity, ent.uiHorizontalGroup);
			if (ent.hasUIVerticalGroup)
				registry.Emplace<UIVerticalGroup>(entity, ent.uiVerticalGroup);
			if (ent.hasUIGridGroup)
				registry.Emplace<UIGridGroup>(entity, ent.uiGridGroup);
			if (ent.hasUILayoutElement)
				registry.Emplace<UILayoutElement>(entity, ent.uiLayoutElement);
			if (ent.hasUISpacer)
				registry.Emplace<UISpacer>(entity, ent.uiSpacer);
			if (ent.hasUIMask)
				registry.Emplace<UIMask>(entity, ent.uiMask);
			if (ent.hasUIFill)
				registry.Emplace<UIFill>(entity, ent.uiFill);
			if (ent.hasHealth)
				registry.Emplace<Health>(entity, ent.health);
			if (ent.hasSpriteAnimator)
				registry.Emplace<SpriteAnimator>(entity, ent.spriteAnimator);
			if (ent.hasCollider2D)
				registry.Emplace<Collider2D>(entity, ent.collider2D);
			if (ent.hasRigidBody2D)
				registry.Emplace<RigidBody2D>(entity, ent.rigidBody2D);
			if (ent.hasPhysicsBody2D)
				registry.Emplace<PhysicsBody2D>(entity, ent.physicsBody2D);
			if (ent.hasMaterial)
				registry.Emplace<Material>(entity, ent.material);
			if (ent.hasMesh)
			{
				Mesh mesh;
				mesh.meshPath = ent.meshPath;
				mesh.materialPath = ent.meshMaterialPath;
				registry.Emplace<Mesh>(entity, std::move(mesh));
			}
		}

		success = true;
		return true;
	}
}
