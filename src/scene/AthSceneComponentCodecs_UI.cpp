#include "AthSceneIOInternal.h"

#include "../utils/StrictParsing.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace AthSceneIO
{
	void ComponentCodecs::UITransformCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UITransform>(entity))
			return;
		const UITransform &component = registry.Get<UITransform>(entity);
		out << "UITRANSFORM "
		    << component.anchorMin.x << " " << component.anchorMin.y << " "
		    << component.anchorMax.x << " " << component.anchorMax.y << " "
		    << component.pivot.x << " " << component.pivot.y << " "
		    << component.anchoredPosition.x << " " << component.anchoredPosition.y << " "
		    << component.sizeDelta.x << " " << component.sizeDelta.y << " "
		    << component.rotation << " "
		    << component.scale.x << " " << component.scale.y << "\n";
	}

	bool ComponentCodecs::UITransformCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUITransform = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "UITransform", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 13u, "UITransform", outError))
		{
			outError = internal::BuildSchemaError(
				"UITransform",
				"13 numeric values: anchorMin2 anchorMax2 pivot2 anchoredPosition2 sizeDelta2 rotation scale2",
				payload);
			return false;
		}

		ent.uiTransform.anchorMin = glm::vec2(values[0], values[1]);
		ent.uiTransform.anchorMax = glm::vec2(values[2], values[3]);
		ent.uiTransform.pivot = glm::vec2(values[4], values[5]);
		ent.uiTransform.anchoredPosition = glm::vec2(values[6], values[7]);
		ent.uiTransform.sizeDelta = glm::vec2(values[8], values[9]);
		ent.uiTransform.rotation = values[10];
		ent.uiTransform.scale = glm::vec2(values[11], values[12]);
		return true;
	}

	void ComponentCodecs::UISpriteCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UISprite>(entity))
			return;
		const UISprite &component = registry.Get<UISprite>(entity);
		out << "UISPRITE "
		    << component.texture.id << " " << component.shader.id << " "
		    << component.tint.x << " " << component.tint.y << " " << component.tint.z << " " << component.tint.w << " "
		    << component.uv.x << " " << component.uv.y << " " << component.uv.z << " " << component.uv.w << " "
		    << component.layer << " " << component.orderInLayer << " "
		    << (component.preserveAspect ? 1 : 0) << " "
		    << std::quoted(ScenePathResolver::ToRelativePathForSave(component.texturePath)) << " "
		    << std::quoted(ScenePathResolver::ToRelativePathForSave(component.materialPath)) << "\n";
	}

	bool ComponentCodecs::UISpriteCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUISprite = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int preserveAspect = 0;
		std::string extra;
		if (!(ls >> ent.uiSprite.texture.id >> ent.uiSprite.shader.id >>
		      ent.uiSprite.tint.x >> ent.uiSprite.tint.y >> ent.uiSprite.tint.z >> ent.uiSprite.tint.w >>
		      ent.uiSprite.uv.x >> ent.uiSprite.uv.y >> ent.uiSprite.uv.z >> ent.uiSprite.uv.w >>
		      ent.uiSprite.layer >> ent.uiSprite.orderInLayer >> preserveAspect >>
		      std::quoted(ent.uiSprite.texturePath) >> std::quoted(ent.uiSprite.materialPath)) ||
		    (ls >> extra))
		{
			outError = internal::BuildSchemaError(
				"UISprite",
				"UISPRITE <texId> <shaderId> <tint4> <uv4> <layer> <orderInLayer> <preserveAspect> \"<texturePath>\" \"<materialPath>\"",
				payload);
			return false;
		}

		ent.uiSprite.preserveAspect = (preserveAspect != 0);
		return true;
	}

	void ComponentCodecs::UITextCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UIText>(entity))
			return;
		const UIText &component = registry.Get<UIText>(entity);
		out << "UITEXT "
		    << std::quoted(component.text) << " "
		    << component.color.x << " " << component.color.y << " " << component.color.z << " " << component.color.w << " "
		    << std::quoted(component.fontId) << " "
		    << component.fontSizePx << " "
		    << static_cast<int>(component.alignment) << " "
		    << (component.wrap ? 1 : 0) << " "
		    << (component.outlineEnabled ? 1 : 0) << " "
		    << component.outlineColor.x << " " << component.outlineColor.y << " " << component.outlineColor.z << " " << component.outlineColor.w << " "
		    << component.outlineThicknessPx << " "
		    << component.layer << " " << component.orderInLayer << "\n";
	}

	bool ComponentCodecs::UITextCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUIText = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int alignment = 0;
		int wrap = 0;
		int outlineEnabled = 0;
		std::string extra;
		if (!(ls >> std::quoted(ent.uiText.text) >>
		      ent.uiText.color.x >> ent.uiText.color.y >> ent.uiText.color.z >> ent.uiText.color.w >>
		      std::quoted(ent.uiText.fontId) >>
		      ent.uiText.fontSizePx >>
		      alignment >> wrap >> outlineEnabled >>
		      ent.uiText.outlineColor.x >> ent.uiText.outlineColor.y >> ent.uiText.outlineColor.z >> ent.uiText.outlineColor.w >>
		      ent.uiText.outlineThicknessPx >>
		      ent.uiText.layer >> ent.uiText.orderInLayer) ||
		    (ls >> extra))
		{
			outError = internal::BuildSchemaError(
				"UIText",
				"UITEXT \"<text>\" <color4> \"<fontId>\" <fontSizePx> <alignment> <wrap> <outlineEnabled> <outlineColor4> <outlineThicknessPx> <layer> <orderInLayer>",
				payload);
			return false;
		}

		if (!internal::IsValidUITextAlignmentValue(alignment))
		{
			outError = internal::BuildSchemaError("UIText", "alignment enum value in [0..2]", payload);
			return false;
		}

		ent.uiText.alignment = internal::UITextAlignmentFromStoredValue(alignment);
		ent.uiText.wrap = (wrap != 0);
		ent.uiText.outlineEnabled = (outlineEnabled != 0);
		return true;
	}

	void ComponentCodecs::UIHorizontalGroupCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UIHorizontalGroup>(entity))
			return;
		const UIHorizontalGroup &component = registry.Get<UIHorizontalGroup>(entity);
		out << "UIHORIZONTALGROUP "
		    << component.padding.left << " " << component.padding.right << " " << component.padding.top << " " << component.padding.bottom << " "
		    << component.spacing << " "
		    << static_cast<int>(component.childAlignment) << " "
		    << (component.expandWidth ? 1 : 0) << " "
		    << (component.expandHeight ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::UIHorizontalGroupCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUIHorizontalGroup = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int alignment = 0;
		int expandWidth = 0;
		int expandHeight = 0;
		std::string extra;
		if (!(ls >> ent.uiHorizontalGroup.padding.left >> ent.uiHorizontalGroup.padding.right >>
		      ent.uiHorizontalGroup.padding.top >> ent.uiHorizontalGroup.padding.bottom >>
		      ent.uiHorizontalGroup.spacing >>
		      alignment >> expandWidth >> expandHeight) ||
		    (ls >> extra))
		{
			outError = internal::BuildSchemaError(
				"UIHorizontalGroup",
				"UIHORIZONTALGROUP <paddingL> <paddingR> <paddingT> <paddingB> <spacing> <alignment> <expandWidth> <expandHeight>",
				payload);
			return false;
		}
		if (!internal::IsValidUIChildAlignmentValue(alignment))
		{
			outError = internal::BuildSchemaError("UIHorizontalGroup", "alignment enum value in [0..2]", payload);
			return false;
		}
		ent.uiHorizontalGroup.childAlignment = internal::UIChildAlignmentFromStoredValue(alignment);
		ent.uiHorizontalGroup.expandWidth = (expandWidth != 0);
		ent.uiHorizontalGroup.expandHeight = (expandHeight != 0);
		return true;
	}

	void ComponentCodecs::UIVerticalGroupCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UIVerticalGroup>(entity))
			return;
		const UIVerticalGroup &component = registry.Get<UIVerticalGroup>(entity);
		out << "UIVERTICALGROUP "
		    << component.padding.left << " " << component.padding.right << " " << component.padding.top << " " << component.padding.bottom << " "
		    << component.spacing << " "
		    << static_cast<int>(component.childAlignment) << " "
		    << (component.expandWidth ? 1 : 0) << " "
		    << (component.expandHeight ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::UIVerticalGroupCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUIVerticalGroup = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int alignment = 0;
		int expandWidth = 0;
		int expandHeight = 0;
		std::string extra;
		if (!(ls >> ent.uiVerticalGroup.padding.left >> ent.uiVerticalGroup.padding.right >>
		      ent.uiVerticalGroup.padding.top >> ent.uiVerticalGroup.padding.bottom >>
		      ent.uiVerticalGroup.spacing >>
		      alignment >> expandWidth >> expandHeight) ||
		    (ls >> extra))
		{
			outError = internal::BuildSchemaError(
				"UIVerticalGroup",
				"UIVERTICALGROUP <paddingL> <paddingR> <paddingT> <paddingB> <spacing> <alignment> <expandWidth> <expandHeight>",
				payload);
			return false;
		}
		if (!internal::IsValidUIChildAlignmentValue(alignment))
		{
			outError = internal::BuildSchemaError("UIVerticalGroup", "alignment enum value in [0..2]", payload);
			return false;
		}
		ent.uiVerticalGroup.childAlignment = internal::UIChildAlignmentFromStoredValue(alignment);
		ent.uiVerticalGroup.expandWidth = (expandWidth != 0);
		ent.uiVerticalGroup.expandHeight = (expandHeight != 0);
		return true;
	}

	void ComponentCodecs::UIGridGroupCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UIGridGroup>(entity))
			return;
		const UIGridGroup &component = registry.Get<UIGridGroup>(entity);
		out << "UIGRIDGROUP "
		    << component.cellSize.x << " " << component.cellSize.y << " "
		    << component.spacing.x << " " << component.spacing.y << " "
		    << static_cast<int>(component.constraint) << " "
		    << component.count << " "
		    << component.padding.left << " " << component.padding.right << " " << component.padding.top << " " << component.padding.bottom << " "
		    << static_cast<int>(component.alignment) << "\n";
	}

	bool ComponentCodecs::UIGridGroupCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUIGridGroup = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int constraint = 0;
		int alignment = 0;
		std::string extra;
		if (!(ls >> ent.uiGridGroup.cellSize.x >> ent.uiGridGroup.cellSize.y >>
		      ent.uiGridGroup.spacing.x >> ent.uiGridGroup.spacing.y >>
		      constraint >>
		      ent.uiGridGroup.count >>
		      ent.uiGridGroup.padding.left >> ent.uiGridGroup.padding.right >>
		      ent.uiGridGroup.padding.top >> ent.uiGridGroup.padding.bottom >>
		      alignment) ||
		    (ls >> extra))
		{
			outError = internal::BuildSchemaError(
				"UIGridGroup",
				"UIGRIDGROUP <cellSize2> <spacing2> <constraint> <count> <padding4> <alignment>",
				payload);
			return false;
		}
		if (!internal::IsValidUIGridConstraintValue(constraint))
		{
			outError = internal::BuildSchemaError("UIGridGroup", "constraint enum value in [0..1]", payload);
			return false;
		}
		if (!internal::IsValidUIChildAlignmentValue(alignment))
		{
			outError = internal::BuildSchemaError("UIGridGroup", "alignment enum value in [0..2]", payload);
			return false;
		}
		ent.uiGridGroup.constraint = internal::UIGridConstraintFromStoredValue(constraint);
		ent.uiGridGroup.alignment = internal::UIChildAlignmentFromStoredValue(alignment);
		ent.uiGridGroup.count = std::max(1, ent.uiGridGroup.count);
		return true;
	}

	void ComponentCodecs::UILayoutElementCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UILayoutElement>(entity))
			return;
		const UILayoutElement &component = registry.Get<UILayoutElement>(entity);
		out << "UILAYOUTELEMENT "
		    << component.minSize.x << " " << component.minSize.y << " "
		    << component.preferredSize.x << " " << component.preferredSize.y << " "
		    << component.flexibleSize.x << " " << component.flexibleSize.y << "\n";
	}

	bool ComponentCodecs::UILayoutElementCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUILayoutElement = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "UILayoutElement", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 6u, "UILayoutElement", outError))
		{
			outError = internal::BuildSchemaError("UILayoutElement", "UILAYOUTELEMENT <min2> <preferred2> <flexible2>", payload);
			return false;
		}
		ent.uiLayoutElement.minSize = glm::vec2(values[0], values[1]);
		ent.uiLayoutElement.preferredSize = glm::vec2(values[2], values[3]);
		ent.uiLayoutElement.flexibleSize = glm::vec2(values[4], values[5]);
		return true;
	}

	void ComponentCodecs::UISpacerCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UISpacer>(entity))
			return;
		const UISpacer &component = registry.Get<UISpacer>(entity);
		out << "UISPACER "
		    << component.preferredSize.x << " " << component.preferredSize.y << " "
		    << component.flexibleSize.x << " " << component.flexibleSize.y << "\n";
	}

	bool ComponentCodecs::UISpacerCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUISpacer = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "UISpacer", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 4u, "UISpacer", outError))
		{
			outError = internal::BuildSchemaError("UISpacer", "UISPACER <preferred2> <flexible2>", payload);
			return false;
		}
		ent.uiSpacer.preferredSize = glm::vec2(values[0], values[1]);
		ent.uiSpacer.flexibleSize = glm::vec2(values[2], values[3]);
		return true;
	}

	void ComponentCodecs::UIMaskCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UIMask>(entity))
			return;
		const UIMask &component = registry.Get<UIMask>(entity);
		out << "UIMASK " << (component.enabled ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::UIMaskCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUIMask = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<int> values;
		std::string parseError;
		if (!StrictParsing::ParseIntList(payload, values, parseError))
		{
			outError = internal::BuildSchemaError("UIMask", "<0|1>", payload) + " " + parseError;
			return false;
		}
		if (!StrictParsing::RequireTokenCount(values, 1u, "UIMask", outError))
		{
			outError = internal::BuildSchemaError("UIMask", "UIMASK <0|1>", payload);
			return false;
		}
		ent.uiMask.enabled = (values[0] != 0);
		return true;
	}

	void ComponentCodecs::UIFillCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<UIFill>(entity))
			return;
		const UIFill &component = registry.Get<UIFill>(entity);
		out << "UIFILL " << component.value01 << " " << static_cast<int>(component.direction) << "\n";
	}

	bool ComponentCodecs::UIFillCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasUIFill = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int direction = 0;
		std::string extra;
		if (!(ls >> ent.uiFill.value01 >> direction) || (ls >> extra))
		{
			outError = internal::BuildSchemaError("UIFill", "UIFILL <value01> <direction>", payload);
			return false;
		}
		if (!internal::IsValidUIFillDirectionValue(direction))
		{
			outError = internal::BuildSchemaError("UIFill", "direction enum value in [0..1]", payload);
			return false;
		}
		ent.uiFill.direction = internal::UIFillDirectionFromStoredValue(direction);
		return true;
	}

	void ComponentCodecs::HealthCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Health>(entity))
			return;
		const Health &component = registry.Get<Health>(entity);
		out << "HEALTH " << component.current << " " << component.max << "\n";
	}

	bool ComponentCodecs::HealthCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasHealth = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "Health", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 2u, "Health", outError))
		{
			outError = internal::BuildSchemaError("Health", "HEALTH <current> <max>", payload);
			return false;
		}
		ent.health.current = values[0];
		ent.health.max = values[1];
		return true;
	}

	void ComponentCodecs::SpriteAnimatorCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<SpriteAnimator>(entity))
			return;
		const SpriteAnimator &component = registry.Get<SpriteAnimator>(entity);
		out << "SPRITE_ANIMATOR "
		    << std::quoted(component.clipId) << " "
		    << component.time << " "
		    << component.fps << " "
		    << component.speed << " "
		    << (component.loop ? 1 : 0) << " "
		    << (component.playing ? 1 : 0) << " "
		    << component.currentFrame << " "
		    << component.columns << " "
		    << component.rows << " "
		    << component.gapX << " "
		    << component.gapY << " "
		    << component.startIndexX << " "
		    << component.startIndexY << " "
		    << component.frameCount << " "
		    << component.marginX << " "
		    << component.marginY << " "
		    << component.cellWidth << " "
		    << component.cellHeight << "\n";
	}

	bool ComponentCodecs::SpriteAnimatorCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasSpriteAnimator = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int loop = 0;
		int playing = 0;
		if (!(ls >> std::quoted(ent.spriteAnimator.clipId) >>
		      ent.spriteAnimator.time))
		{
			outError = internal::BuildSchemaError(
				"SpriteAnimator",
				"SPRITE_ANIMATOR \"<clipId>\" <time> ...",
				payload);
			return false;
		}

		std::vector<float> numericTail;
		float token = 0.f;
		while (ls >> token)
			numericTail.push_back(token);
		if (!ls.eof() && ls.fail())
		{
			outError = internal::BuildSchemaError("SpriteAnimator", "numeric tail after <time>", payload);
			return false;
		}

		if (numericTail.size() < 5u)
		{
			outError = internal::BuildSchemaError(
				"SpriteAnimator",
				"at least 5 numeric values after <time>: <fps> <speed> <loop> <playing> <currentFrame>",
				payload);
			return false;
		}

		ent.spriteAnimator.fps = numericTail[0];
		ent.spriteAnimator.speed = numericTail[1];
		loop = static_cast<int>(numericTail[2]);
		playing = static_cast<int>(numericTail[3]);
		ent.spriteAnimator.currentFrame = static_cast<int>(numericTail[4]);

		if (numericTail.size() > 5u)
			ent.spriteAnimator.columns = static_cast<int>(numericTail[5]);
		if (numericTail.size() > 6u)
			ent.spriteAnimator.rows = static_cast<int>(numericTail[6]);
		if (numericTail.size() > 7u)
			ent.spriteAnimator.gapX = static_cast<int>(numericTail[7]);
		if (numericTail.size() > 8u)
			ent.spriteAnimator.gapY = static_cast<int>(numericTail[8]);
		if (numericTail.size() > 9u)
			ent.spriteAnimator.startIndexX = static_cast<int>(numericTail[9]);
		if (numericTail.size() > 10u)
			ent.spriteAnimator.startIndexY = static_cast<int>(numericTail[10]);
		if (numericTail.size() > 11u)
			ent.spriteAnimator.frameCount = static_cast<int>(numericTail[11]);
		if (numericTail.size() > 12u)
			ent.spriteAnimator.marginX = static_cast<int>(numericTail[12]);
		if (numericTail.size() > 13u)
			ent.spriteAnimator.marginY = static_cast<int>(numericTail[13]);
		if (numericTail.size() > 14u)
			ent.spriteAnimator.cellWidth = static_cast<int>(numericTail[14]);
		if (numericTail.size() > 15u)
			ent.spriteAnimator.cellHeight = static_cast<int>(numericTail[15]);

		ent.spriteAnimator.loop = (loop != 0);
		ent.spriteAnimator.playing = (playing != 0);
		ent.spriteAnimator.fps = std::max(0.0f, ent.spriteAnimator.fps);
		ent.spriteAnimator.columns = std::max(1, ent.spriteAnimator.columns);
		ent.spriteAnimator.rows = std::max(1, ent.spriteAnimator.rows);
		ent.spriteAnimator.gapX = std::max(0, ent.spriteAnimator.gapX);
		ent.spriteAnimator.gapY = std::max(0, ent.spriteAnimator.gapY);
		ent.spriteAnimator.marginX = std::max(0, ent.spriteAnimator.marginX);
		ent.spriteAnimator.marginY = std::max(0, ent.spriteAnimator.marginY);
		ent.spriteAnimator.cellWidth = std::max(0, ent.spriteAnimator.cellWidth);
		ent.spriteAnimator.cellHeight = std::max(0, ent.spriteAnimator.cellHeight);
		return true;
	}
}
