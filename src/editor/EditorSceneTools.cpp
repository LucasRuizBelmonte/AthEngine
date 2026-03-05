#pragma region Includes
#include "EditorSceneTools.h"

#include <imgui.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <format>

#include "../scene/IEditorScene.h"
#include "../components/Tag.h"
#include "../components/Parent.h"

#include "../components/Transform.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Sprite.h"
#include "../components/Spin.h"
#include "../components/LightEmitter.h"
#include "../animation2d/SpriteAnimator.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/PhysicsBody2D.h"
#include "../physics2d/RigidBody2D.h"
#include "../material/MaterialMetadata.h"
#pragma endregion

#pragma region Function Definitions
namespace sceneeditor
{
	static constexpr const char *kAssetPickerPopupId = "Asset Browser";

	enum class AssetPickerType : int
	{
		None = 0,
		Texture,
		Material,
		Mesh
	};

	struct AssetPickerRuntime
	{
		bool openRequested = false;
		AssetPickerType type = AssetPickerType::None;
		Entity entity = kInvalidEntity;
		std::string fieldKey;
		std::vector<std::string> entries;
		char filter[128] = {};
		int selection = -1;

		bool commitPending = false;
		Entity commitEntity = kInvalidEntity;
		std::string commitFieldKey;
		std::string commitPath;
	};

	struct BufferedStringEditState
	{
		bool editing = false;
		std::string originalValue;
		std::string buffer;
		std::vector<char> inputBuffer;
	};

	static std::unordered_map<std::string, BufferedStringEditState> s_bufferedStringEdits;
	static float s_dragSnapStep = 0.5f;

	static std::string BuildBufferedStringEditKey(Entity entity, const char *fieldKey)
	{
		return std::to_string(EntityIdOf(entity)) + ":" + (fieldKey ? fieldKey : "");
	}

	static void ResetBufferedStringEdit(Entity entity, const char *fieldKey, const std::string &value)
	{
		BufferedStringEditState &state = s_bufferedStringEdits[BuildBufferedStringEditKey(entity, fieldKey)];
		state.editing = false;
		state.originalValue = value;
		state.buffer = value;
		state.inputBuffer.assign(std::max<size_t>(state.buffer.size() + 1u, 64u), '\0');
		std::snprintf(state.inputBuffer.data(), state.inputBuffer.size(), "%s", state.buffer.c_str());
	}

	static bool DrawBufferedStringInput(Entity entity,
										const char *fieldKey,
										const char *inputLabel,
										std::string &inOutValue,
										size_t minCapacity)
	{
		BufferedStringEditState &state = s_bufferedStringEdits[BuildBufferedStringEditKey(entity, fieldKey)];
		if (!state.editing)
		{
			state.originalValue = inOutValue;
			state.buffer = inOutValue;
		}

		const size_t capacity = std::max(minCapacity, state.buffer.size() + 1u);
		if (state.inputBuffer.size() < capacity)
			state.inputBuffer.resize(capacity, '\0');

		std::snprintf(state.inputBuffer.data(), state.inputBuffer.size(), "%s", state.buffer.c_str());

		const bool enterPressed = ImGui::InputText(inputLabel,
												   state.inputBuffer.data(),
												   state.inputBuffer.size(),
												   ImGuiInputTextFlags_EnterReturnsTrue);

		if (ImGui::IsItemActivated())
		{
			state.editing = true;
			state.originalValue = inOutValue;
			state.buffer = inOutValue;
		}

		if (ImGui::IsItemActive())
		{
			if (!state.editing)
			{
				state.editing = true;
				state.originalValue = inOutValue;
			}
			state.buffer = state.inputBuffer.data();
		}

		if (state.editing && ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			state.buffer = state.originalValue;
			state.editing = false;
			return false;
		}

		if (enterPressed)
		{
			state.editing = false;
			const bool changed = (inOutValue != state.buffer);
			inOutValue = state.buffer;
			state.originalValue = inOutValue;
			state.buffer = inOutValue;
			return changed;
		}

		if (state.editing && ImGui::IsItemDeactivated())
		{
			state.buffer = state.originalValue;
			state.editing = false;
		}

		return false;
	}

	static bool ShouldSnapDraggedValues()
	{
		return s_dragSnapStep > 0.0f && ImGui::GetIO().KeyCtrl && ImGui::IsItemActive();
	}

	static float SnapToStep(float value, float step)
	{
		if (step <= 0.0f)
			return value;
		return std::round(value / step) * step;
	}

	static float SnapToStepDirectional(float value, float step, float mouseDeltaX)
	{
		if (step <= 0.0f)
			return value;

		const float thresholdFraction = 1.0f;

		const float scaled = value / step;
		const float floored = std::floor(scaled);
		const float lowerStep = floored * step;
		const float upperStep = lowerStep + step;
		const float fraction = scaled - floored;

		if (mouseDeltaX > 0.0f)
			return (fraction >= thresholdFraction) ? upperStep : lowerStep;
		if (mouseDeltaX < 0.0f)
			return (fraction <= (1.0f - thresholdFraction)) ? lowerStep : upperStep;

		return SnapToStep(value, step);
	}

	static void ApplySnapToFloat(float &value, float minValue = 0.0f, float maxValue = 0.0f)
	{
		if (!ShouldSnapDraggedValues())
			return;

		const float mouseDeltaX = ImGui::GetIO().MouseDelta.x;
		value = SnapToStepDirectional(value, s_dragSnapStep, mouseDeltaX);
		if (maxValue > minValue)
			value = std::clamp(value, minValue, maxValue);
	}

	static bool DragFloatWithSnap(const char *label,
								  float *v,
								  float speed,
								  float minValue = 0.0f,
								  float maxValue = 0.0f,
								  const char *format = "%.3f",
								  ImGuiSliderFlags flags = 0)
	{
		const bool changed = ImGui::DragFloat(label, v, speed, minValue, maxValue, format, flags);
		if (changed)
			ApplySnapToFloat(*v, minValue, maxValue);
		return changed;
	}

	static bool DragFloat2WithSnap(const char *label,
								   float *v,
								   float speed,
								   const char *format = "%.3f",
								   ImGuiSliderFlags flags = 0)
	{
		const bool changed = ImGui::DragFloat2(label, v, speed, 0.0f, 0.0f, format, flags);
		if (changed && ShouldSnapDraggedValues())
		{
			const float mouseDeltaX = ImGui::GetIO().MouseDelta.x;
			v[0] = SnapToStepDirectional(v[0], s_dragSnapStep, mouseDeltaX);
			v[1] = SnapToStepDirectional(v[1], s_dragSnapStep, mouseDeltaX);
		}
		return changed;
	}

	static bool DragFloat3WithSnap(const char *label,
								   float *v,
								   float speed,
								   float minValue = 0.0f,
								   float maxValue = 0.0f,
								   const char *format = "%.3f",
								   ImGuiSliderFlags flags = 0)
	{
		const bool changed = ImGui::DragFloat3(label, v, speed, minValue, maxValue, format, flags);
		if (changed && ShouldSnapDraggedValues())
		{
			const float mouseDeltaX = ImGui::GetIO().MouseDelta.x;
			v[0] = SnapToStepDirectional(v[0], s_dragSnapStep, mouseDeltaX);
			v[1] = SnapToStepDirectional(v[1], s_dragSnapStep, mouseDeltaX);
			v[2] = SnapToStepDirectional(v[2], s_dragSnapStep, mouseDeltaX);
			if (maxValue > minValue)
			{
				v[0] = std::clamp(v[0], minValue, maxValue);
				v[1] = std::clamp(v[1], minValue, maxValue);
				v[2] = std::clamp(v[2], minValue, maxValue);
			}
		}
		return changed;
	}

	static bool DragFloat4WithSnap(const char *label,
								   float *v,
								   float speed,
								   const char *format = "%.3f",
								   ImGuiSliderFlags flags = 0)
	{
		const bool changed = ImGui::DragFloat4(label, v, speed, 0.0f, 0.0f, format, flags);
		if (changed && ShouldSnapDraggedValues())
		{
			const float mouseDeltaX = ImGui::GetIO().MouseDelta.x;
			v[0] = SnapToStepDirectional(v[0], s_dragSnapStep, mouseDeltaX);
			v[1] = SnapToStepDirectional(v[1], s_dragSnapStep, mouseDeltaX);
			v[2] = SnapToStepDirectional(v[2], s_dragSnapStep, mouseDeltaX);
			v[3] = SnapToStepDirectional(v[3], s_dragSnapStep, mouseDeltaX);
		}
		return changed;
	}

	static const char *AssetPickerTypeLabel(AssetPickerType type)
	{
		switch (type)
		{
		case AssetPickerType::Texture:
			return "Texture";
		case AssetPickerType::Material:
			return "Material";
		case AssetPickerType::Mesh:
			return "Mesh";
		case AssetPickerType::None:
		default:
			return "Unknown";
		}
	}

	static std::string ToLowerCopy(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
					   { return static_cast<char>(std::tolower(c)); });
		return text;
	}

	static std::string NormalizePathSeparators(std::string path)
	{
		std::replace(path.begin(), path.end(), '\\', '/');
		return path;
	}

	static bool PathMatchesAssetType(const std::filesystem::path &path, AssetPickerType type)
	{
		const std::string ext = ToLowerCopy(path.extension().string());

		switch (type)
		{
		case AssetPickerType::Texture:
			return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" ||
				   ext == ".gif" || ext == ".hdr" || ext == ".psd";
		case AssetPickerType::Material:
			return ext == ".fs" || ext == ".frag" || ext == ".glsl";
		case AssetPickerType::Mesh:
			return ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".gltf" || ext == ".glb" ||
				   ext == ".3ds" || ext == ".blend";
		case AssetPickerType::None:
		default:
			return false;
		}
	}

	static std::vector<std::string> CollectAssetPickerEntries(AssetPickerType type)
	{
		std::vector<std::string> paths;
		if (type == AssetPickerType::None)
			return paths;

		std::error_code ec;
		const std::filesystem::path assetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
		const std::filesystem::path projectRoot = assetRoot.parent_path();

		if (!std::filesystem::exists(assetRoot, ec))
			return paths;

		std::filesystem::recursive_directory_iterator it(assetRoot, std::filesystem::directory_options::skip_permission_denied, ec);
		const std::filesystem::recursive_directory_iterator end;
		for (; it != end; it.increment(ec))
		{
			if (ec)
			{
				ec.clear();
				continue;
			}

			if (!it->is_regular_file(ec))
			{
				if (ec)
					ec.clear();
				continue;
			}

			const std::filesystem::path candidate = it->path();
			if (!PathMatchesAssetType(candidate, type))
				continue;

			std::filesystem::path rel = std::filesystem::relative(candidate, projectRoot, ec);
			if (ec)
			{
				ec.clear();
				rel = candidate.lexically_normal();
			}

			paths.push_back(rel.generic_string());
		}

		std::sort(paths.begin(), paths.end());
		return paths;
	}

	static void RequestAssetPicker(AssetPickerRuntime &picker,
								   Entity entity,
								   const char *fieldKey,
								   AssetPickerType type,
								   const std::string &currentValue)
	{
		picker.openRequested = true;
		picker.entity = entity;
		picker.fieldKey = fieldKey ? fieldKey : "";
		picker.type = type;
		picker.entries = CollectAssetPickerEntries(type);
		picker.filter[0] = '\0';
		picker.selection = -1;

		const std::string normalizedCurrent = NormalizePathSeparators(currentValue);
		for (size_t i = 0; i < picker.entries.size(); ++i)
		{
			if (NormalizePathSeparators(picker.entries[i]) == normalizedCurrent)
			{
				picker.selection = static_cast<int>(i);
				break;
			}
		}
	}

	static bool ConsumeAssetPickerCommit(AssetPickerRuntime &picker,
										 Entity entity,
										 const char *fieldKey,
										 std::string &inOutValue)
	{
		if (!picker.commitPending)
			return false;
		if (picker.commitEntity != entity)
			return false;
		if (picker.commitFieldKey != (fieldKey ? fieldKey : ""))
			return false;

		inOutValue = picker.commitPath;
		ResetBufferedStringEdit(entity, fieldKey, inOutValue);

		picker.commitPending = false;
		picker.commitEntity = kInvalidEntity;
		picker.commitFieldKey.clear();
		picker.commitPath.clear();
		return true;
	}

	static bool DrawPathFieldWithAssetPicker(AssetPickerRuntime &picker,
											 Entity entity,
											 const char *label,
											 const char *fieldKey,
											 AssetPickerType type,
											 std::string &inOutValue)
	{
		bool changed = ConsumeAssetPickerCommit(picker, entity, fieldKey, inOutValue);

		ImGui::PushID(fieldKey);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine();

		const float buttonWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		const float avail = ImGui::GetContentRegionAvail().x;
		const float inputWidth = std::max(80.0f, avail - buttonWidth - spacing);
		ImGui::SetNextItemWidth(inputWidth);

		if (DrawBufferedStringInput(entity, fieldKey, "##Path", inOutValue, 512))
		{
			changed = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("..."))
			RequestAssetPicker(picker, entity, fieldKey, type, inOutValue);

		ImGui::PopID();
		return changed;
	}

	static void DrawAssetPickerPopup(AssetPickerRuntime &picker)
	{
		if (picker.openRequested)
		{
			ImGui::OpenPopup(kAssetPickerPopupId);
			picker.openRequested = false;
		}

		if (!ImGui::BeginPopupModal(kAssetPickerPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		ImGui::Text("Type: %s", AssetPickerTypeLabel(picker.type));
		ImGui::InputTextWithHint("Filter", "Type to filter...", picker.filter, sizeof(picker.filter));

		const std::string filter = ToLowerCopy(picker.filter);
		int shownCount = 0;

		ImGui::BeginChild("AssetList", ImVec2(740.f, 320.f), true);
		for (int i = 0; i < static_cast<int>(picker.entries.size()); ++i)
		{
			const std::string &path = picker.entries[(size_t)i];
			if (!filter.empty())
			{
				const std::string lowerPath = ToLowerCopy(path);
				if (lowerPath.find(filter) == std::string::npos)
					continue;
			}

			++shownCount;
			const bool isSelected = (picker.selection == i);
			if (ImGui::Selectable(path.c_str(), isSelected))
				picker.selection = i;
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndChild();

		if (shownCount == 0)
			ImGui::TextUnformatted("No matching assets.");

		const bool canSelect = picker.selection >= 0 &&
							   picker.selection < static_cast<int>(picker.entries.size());
		if (!canSelect)
			ImGui::BeginDisabled();
		if (ImGui::Button("Select"))
		{
			picker.commitPending = true;
			picker.commitEntity = picker.entity;
			picker.commitFieldKey = picker.fieldKey;
			picker.commitPath = picker.entries[(size_t)picker.selection];
			ImGui::CloseCurrentPopup();
		}
		if (!canSelect)
			ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			picker.commitPending = true;
			picker.commitEntity = picker.entity;
			picker.commitFieldKey = picker.fieldKey;
			picker.commitPath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	static const char *EntityLabel(Registry &r, Entity e, char *tmp, int tmpSize)
	{
		if (r.Has<Tag>(e))
		{
			const auto &t = r.Get<Tag>(e);
			if (!t.name.empty())
				return t.name.c_str();
		}
		std::snprintf(tmp, (size_t)tmpSize, "Entity %u", EntityIdOf(e));
		return tmp;
	}

	static Entity GetParentOf(Registry &r, Entity e)
	{
		if (!r.Has<Parent>(e))
			return kInvalidEntity;
		return r.Get<Parent>(e).parent;
	}

	struct ResolvedTransformChannels
	{
		glm::vec3 position{0.f, 0.f, 0.f};
		glm::vec3 rotation{0.f, 0.f, 0.f};
		glm::vec3 scale{1.f, 1.f, 1.f};
	};

	static Entity GetAliveParentWithTransformOf(Registry &r, Entity e)
	{
		const Entity parent = GetParentOf(r, e);
		if (parent == kInvalidEntity || !r.IsAlive(parent) || !r.Has<Transform>(parent))
			return kInvalidEntity;
		return parent;
	}

	static ResolvedTransformChannels ResolveWorldChannels(Registry &r, Entity e)
	{
		ResolvedTransformChannels out;
		if (e == kInvalidEntity || !r.IsAlive(e) || !r.Has<Transform>(e))
			return out;

		std::vector<Entity> chain;
		Entity current = e;
		for (int i = 0; i < 256 && current != kInvalidEntity && r.IsAlive(current); ++i)
		{
			if (!r.Has<Transform>(current))
				break;

			chain.push_back(current);
			current = GetAliveParentWithTransformOf(r, current);
		}

		for (auto it = chain.rbegin(); it != chain.rend(); ++it)
		{
			const Transform &local = r.Get<Transform>(*it);
			out.position = local.absolutePosition ? local.localPosition : (out.position + local.localPosition);
			out.rotation = local.absoluteRotation ? local.localRotation : (out.rotation + local.localRotation);
			out.scale = local.absoluteScale ? local.localScale : (out.scale * local.localScale);
		}

		return out;
	}

	static glm::vec3 DivideScaleSafe(const glm::vec3 &lhs, const glm::vec3 &rhs)
	{
		auto divideAxis = [](float left, float right)
		{
			return (std::abs(right) > 1e-6f) ? (left / right) : left;
		};

		return glm::vec3{
			divideAxis(lhs.x, rhs.x),
			divideAxis(lhs.y, rhs.y),
			divideAxis(lhs.z, rhs.z)};
	}

	static void ApplyAbsoluteFlagsPreservingWorld(Registry &r,
												  Entity e,
												  bool absolutePosition,
												  bool absoluteRotation,
												  bool absoluteScale)
	{
		if (e == kInvalidEntity || !r.IsAlive(e) || !r.Has<Transform>(e))
			return;

		Transform &t = r.Get<Transform>(e);
		const ResolvedTransformChannels world = ResolveWorldChannels(r, e);

		const Entity parent = GetAliveParentWithTransformOf(r, e);
		const bool hasParent = (parent != kInvalidEntity);
		const ResolvedTransformChannels parentWorld = hasParent ? ResolveWorldChannels(r, parent) : ResolvedTransformChannels{};

		t.absolutePosition = absolutePosition;
		t.absoluteRotation = absoluteRotation;
		t.absoluteScale = absoluteScale;

		t.localPosition =
			(absolutePosition || !hasParent)
				? world.position
				: (world.position - parentWorld.position);
		t.localRotation =
			(absoluteRotation || !hasParent)
				? world.rotation
				: (world.rotation - parentWorld.rotation);
		t.localScale =
			(absoluteScale || !hasParent)
				? world.scale
				: DivideScaleSafe(world.scale, parentWorld.scale);
	}

	static void SetParentOfRaw(Registry &r, Entity e, Entity parent)
	{
		if (!r.Has<Parent>(e))
			r.Emplace<Parent>(e, Parent{parent});
		else
			r.Get<Parent>(e).parent = parent;
	}

	static void ReparentEntityPreservingWorldTransform(Registry &r, Entity e, Entity newParent)
	{
		if (e == kInvalidEntity || !r.IsAlive(e))
			return;

		Entity targetParent = newParent;
		if (targetParent != kInvalidEntity && (!r.IsAlive(targetParent) || targetParent == e))
			targetParent = kInvalidEntity;

		const bool hasTransform = r.Has<Transform>(e);
		ResolvedTransformChannels worldBefore{};
		if (hasTransform)
			worldBefore = ResolveWorldChannels(r, e);

		SetParentOfRaw(r, e, targetParent);

		if (!hasTransform)
			return;

		Transform &t = r.Get<Transform>(e);
		const Entity parentWithTransform = GetAliveParentWithTransformOf(r, e);
		const bool hasParentWithTransform = (parentWithTransform != kInvalidEntity);
		const ResolvedTransformChannels parentWorld =
			hasParentWithTransform ? ResolveWorldChannels(r, parentWithTransform) : ResolvedTransformChannels{};

		t.localPosition =
			(t.absolutePosition || !hasParentWithTransform)
				? worldBefore.position
				: (worldBefore.position - parentWorld.position);
		t.localRotation =
			(t.absoluteRotation || !hasParentWithTransform)
				? worldBefore.rotation
				: (worldBefore.rotation - parentWorld.rotation);
		t.localScale =
			(t.absoluteScale || !hasParentWithTransform)
				? worldBefore.scale
				: DivideScaleSafe(worldBefore.scale, parentWorld.scale);
	}

	static bool IsDescendant(Registry &r, Entity candidateParent, Entity child)
	{
		Entity cur = candidateParent;
		while (cur != kInvalidEntity)
		{
			if (cur == child)
				return true;
			cur = GetParentOf(r, cur);
		}
		return false;
	}

	static void BuildTree(Registry &r,
						  std::unordered_map<Entity, std::vector<Entity>> &children,
						  std::vector<Entity> &roots)
	{
		children.clear();
		roots.clear();

		const auto &alive = r.Alive();
		roots.reserve(alive.size());

		for (Entity e : alive)
		{
			Entity p = GetParentOf(r, e);
			if (p == kInvalidEntity || !r.IsAlive(p))
			{
				roots.push_back(e);
			}
			else
			{
				children[p].push_back(e);
			}
		}
	}

	Entity EditorSceneTools::CreateEntity(Registry &r, const char *name, Entity parent, bool addTransform)
	{
		Entity e = r.Create();

		r.Emplace<Tag>(e, Tag{std::string(name ? name : "Entity")});
		r.Emplace<Parent>(e, Parent{parent});

		if (addTransform)
			r.Emplace<Transform>(e);

		return e;
	}

	static void CollectSubtree(
		Entity e,
		const std::unordered_map<Entity, std::vector<Entity>> &children,
		std::vector<Entity> &out)
	{
		out.push_back(e);

		auto it = children.find(e);
		if (it == children.end())
			return;

		for (Entity c : it->second)
			CollectSubtree(c, children, out);
	}

	void EditorSceneTools::DestroyEntityRecursive(Registry &r, Entity e)
	{
		if (e == kInvalidEntity || !r.IsAlive(e))
			return;

		std::unordered_map<Entity, std::vector<Entity>> children;
		std::vector<Entity> roots;
		BuildTree(r, children, roots);

		std::vector<Entity> toDestroy;
		CollectSubtree(e, children, toDestroy);

		for (auto it = toDestroy.rbegin(); it != toDestroy.rend(); ++it)
			r.Destroy(*it);
	}

	static void CopyTagRename(Registry &r, Entity src, Entity dst)
	{
		if (!r.Has<Tag>(src))
			return;

		auto t = r.Get<Tag>(src);
		t.name += " Copy";
		r.Emplace<Tag>(dst, t);
	}

	template <typename T>
	static void CopyIfPresent(Registry &r, Entity src, Entity dst)
	{
		if (r.Has<T>(src))
			r.Emplace<T>(dst, r.Get<T>(src));
	}

	static Entity DuplicateRecurse(
		Registry &r,
		Entity src,
		Entity parentDst,
		const std::unordered_map<Entity, std::vector<Entity>> &children)
	{
		Entity dst = r.Create();

		r.Emplace<Parent>(dst, Parent{parentDst});
		CopyTagRename(r, src, dst);

		CopyIfPresent<Transform>(r, src, dst);
		CopyIfPresent<Camera>(r, src, dst);
		CopyIfPresent<CameraController>(r, src, dst);
		CopyIfPresent<Material>(r, src, dst);
		CopyIfPresent<Sprite>(r, src, dst);
		CopyIfPresent<SpriteAnimator>(r, src, dst);
		CopyIfPresent<Spin>(r, src, dst);
		CopyIfPresent<LightEmitter>(r, src, dst);
		CopyIfPresent<Collider2D>(r, src, dst);
		CopyIfPresent<RigidBody2D>(r, src, dst);
		CopyIfPresent<PhysicsBody2D>(r, src, dst);

		auto it = children.find(src);
		if (it != children.end())
		{
			for (Entity c : it->second)
				(void)DuplicateRecurse(r, c, dst, children);
		}

		return dst;
	}

	Entity EditorSceneTools::DuplicateEntityRecursive(Registry &r, Entity src)
	{
		if (src == kInvalidEntity || !r.IsAlive(src))
			return kInvalidEntity;

		std::unordered_map<Entity, std::vector<Entity>> children;
		std::vector<Entity> roots;
		BuildTree(r, children, roots);

		Entity parent = GetParentOf(r, src);
		if (parent != kInvalidEntity && !r.IsAlive(parent))
			parent = kInvalidEntity;

		return DuplicateRecurse(r, src, parent, children);
	}

	void EditorSceneTools::BeginRename(Registry &r, SceneEditorState &st, Entity e)
	{
		if (e == kInvalidEntity || !r.IsAlive(e))
			return;

		st.renamingEntity = e;
		st.renameFocus = true;

		const char *name = "Entity";
		if (r.Has<Tag>(e))
			name = r.Get<Tag>(e).name.c_str();

		std::snprintf(st.renameBuf, sizeof(st.renameBuf), "%s", name);
	}

	static bool RenameWidget(Registry &r, SceneEditorState &st, Entity e)
	{
		if (st.renamingEntity != e)
			return false;

		if (st.renameFocus)
		{
			ImGui::SetKeyboardFocusHere();
			st.renameFocus = false;
		}

		bool commit = false;

		if (ImGui::InputText("##rename", st.renameBuf, sizeof(st.renameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
			commit = true;

		if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			st.renamingEntity = kInvalidEntity;
			return true;
		}

		if (commit)
		{
			if (!r.Has<Tag>(e))
				r.Emplace<Tag>(e);

			r.Get<Tag>(e).name = std::string(st.renameBuf);
			st.renamingEntity = kInvalidEntity;
			return true;
		}

		return true;
	}

	static void DrawEntityNode(
		Registry &r,
		Entity e,
		const std::unordered_map<Entity, std::vector<Entity>> &children,
		SceneEditorState &st)
	{
		auto it = children.find(e);
		const bool hasChildren = (it != children.end() && !it->second.empty());

		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_OpenOnArrow;

		if (!hasChildren)
			flags |= ImGuiTreeNodeFlags_Leaf;

		if (st.selectedEntity == e)
			flags |= ImGuiTreeNodeFlags_Selected;

		ImGui::PushID((int)e);

		bool open = ImGui::TreeNodeEx("##node", flags);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			st.selectedEntity = e;

		ImGui::SameLine();

		if (st.renamingEntity == e)
		{
			(void)RenameWidget(r, st, e);
		}
		else
		{
			char tmp[64];
			const char *label = EntityLabel(r, e, tmp, 64);

			ImGuiSelectableFlags sflags = ImGuiSelectableFlags_AllowDoubleClick;
			const ImVec2 selectableSize(ImGui::GetContentRegionAvail().x, 0.0f);
			if (ImGui::Selectable(label, st.selectedEntity == e, sflags, selectableSize))
				st.selectedEntity = e;

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				EditorSceneTools::BeginRename(r, st, e);

			if (ImGui::BeginPopupContextItem("ctx"))
			{
				if (ImGui::MenuItem("Create Child"))
					st.selectedEntity = EditorSceneTools::CreateEntity(r, "New Entity", e, true);

				if (ImGui::MenuItem("Duplicate"))
					st.selectedEntity = EditorSceneTools::DuplicateEntityRecursive(r, e);

				if (ImGui::MenuItem("Rename"))
					EditorSceneTools::BeginRename(r, st, e);

				if (ImGui::MenuItem("Delete"))
				{
					EditorSceneTools::DestroyEntityRecursive(r, e);
					if (st.selectedEntity == e)
						st.selectedEntity = kInvalidEntity;
					if (st.renamingEntity == e)
						st.renamingEntity = kInvalidEntity;
				}

				ImGui::EndPopup();
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				Entity payload = e;
				ImGui::SetDragDropPayload("ATH_ENTITY", &payload, sizeof(Entity));
				ImGui::TextUnformatted(label);
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *p = ImGui::AcceptDragDropPayload("ATH_ENTITY"))
				{
					Entity dropped = *(const Entity *)p->Data;
					if (dropped != e && !IsDescendant(r, e, dropped))
						ReparentEntityPreservingWorldTransform(r, dropped, e);
				}
				ImGui::EndDragDropTarget();
			}
		}

		if (open)
		{
			if (hasChildren)
			{
				for (Entity c : it->second)
					DrawEntityNode(r, c, children, st);
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void EditorSceneTools::DrawHierarchy(Registry &r, SceneEditorState &st)
	{
		if (ImGui::Button("Add Entity"))
			st.selectedEntity = CreateEntity(r, "New Entity", kInvalidEntity, true);

		ImGui::SameLine();

		bool canAct = (st.selectedEntity != kInvalidEntity && r.IsAlive(st.selectedEntity));
		if (!canAct)
			ImGui::BeginDisabled();

		if (ImGui::Button("Add Child"))
			st.selectedEntity = CreateEntity(r, "New Entity", st.selectedEntity, true);

		ImGui::SameLine();
		if (ImGui::Button("Duplicate"))
			st.selectedEntity = DuplicateEntityRecursive(r, st.selectedEntity);

		ImGui::SameLine();
		if (ImGui::Button("Delete"))
		{
			DestroyEntityRecursive(r, st.selectedEntity);
			st.selectedEntity = kInvalidEntity;
			st.renamingEntity = kInvalidEntity;
		}

		ImGui::SameLine();
		if (ImGui::Button("Rename"))
			BeginRename(r, st, st.selectedEntity);

		if (!canAct)
			ImGui::EndDisabled();

		ImGui::Separator();

		std::unordered_map<Entity, std::vector<Entity>> children;
		std::vector<Entity> roots;
		BuildTree(r, children, roots);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload *p = ImGui::AcceptDragDropPayload("ATH_ENTITY"))
			{
				Entity dropped = *(const Entity *)p->Data;
				ReparentEntityPreservingWorldTransform(r, dropped, kInvalidEntity);
			}
			ImGui::EndDragDropTarget();
		}

		for (Entity e : roots)
			DrawEntityNode(r, e, children, st);
	}

	static void DrawVec3(const char *label, float *v, float speed)
	{
		(void)DragFloat3WithSnap(label, v, speed);
	}

	static float WrapDegrees360(float degrees)
	{
		float wrapped = std::fmod(degrees, 360.0f);
		if (wrapped < 0.0f)
			wrapped += 360.0f;
		return wrapped;
	}

	static void DrawRotationEulerDegrees(const char *label, glm::vec3 &rotationEulerRadians, float speedDeg)
	{
		glm::vec3 deg{
			WrapDegrees360(glm::degrees(rotationEulerRadians.x)),
			WrapDegrees360(glm::degrees(rotationEulerRadians.y)),
			WrapDegrees360(glm::degrees(rotationEulerRadians.z))};

		if (DragFloat3WithSnap(label,
							   &deg.x,
							   speedDeg,
							   0.0f,
							   360.0f,
							   "%.3f",
							   ImGuiSliderFlags_AlwaysClamp))
		{
			deg.x = WrapDegrees360(deg.x);
			deg.y = WrapDegrees360(deg.y);
			deg.z = WrapDegrees360(deg.z);
			rotationEulerRadians = glm::radians(deg);
		}
	}

	static void DrawVec2(const char *label, float *v, float speed)
	{
		(void)DragFloat2WithSnap(label, v, speed);
	}

	static bool DrawToggleButton(const char *label, bool &value)
	{
		const ImVec4 onColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		const ImVec4 onHoverColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		const ImVec4 offColor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
		const ImVec4 offHoverColor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered);
		const ImVec4 offActiveColor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive);

		if (value)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, onColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, onHoverColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, onColor);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, offColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, offHoverColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, offActiveColor);
		}

		const bool pressed = ImGui::Button(label);
		ImGui::PopStyleColor(3);

		if (pressed)
			value = !value;

		return pressed;
	}

	static bool DrawInputIntPairHorizontal(const char *leftLabel,
										   int *leftValue,
										   const char *rightLabel,
										   int *rightValue)
	{
		bool changed = false;
		ImGui::PushID(leftLabel);
		if (ImGui::BeginTable("##IntPair", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextWrapped("%s", leftLabel);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			changed |= ImGui::InputInt("##LeftValue", leftValue);

			ImGui::TableSetColumnIndex(1);
			ImGui::TextWrapped("%s", rightLabel);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			changed |= ImGui::InputInt("##RightValue", rightValue);

			ImGui::EndTable();
		}
		ImGui::PopID();
		return changed;
	}

	static void DrawColor4(const char *label, float *v)
	{
		ImGui::ColorEdit4(label, v);
	}

	static int SpritePivotToIndex(SpritePivot pivot)
	{
		switch (pivot)
		{
		case SpritePivot::TopLeft:
			return 1;
		case SpritePivot::Top:
			return 2;
		case SpritePivot::TopRight:
			return 3;
		case SpritePivot::Left:
			return 4;
		case SpritePivot::Right:
			return 5;
		case SpritePivot::BottomLeft:
			return 6;
		case SpritePivot::Bottom:
			return 7;
		case SpritePivot::BottomRight:
			return 8;
		case SpritePivot::Center:
		default:
			return 0;
		}
	}

	static SpritePivot SpritePivotFromIndex(int idx)
	{
		switch (idx)
		{
		case 1:
			return SpritePivot::TopLeft;
		case 2:
			return SpritePivot::Top;
		case 3:
			return SpritePivot::TopRight;
		case 4:
			return SpritePivot::Left;
		case 5:
			return SpritePivot::Right;
		case 6:
			return SpritePivot::BottomLeft;
		case 7:
			return SpritePivot::Bottom;
		case 8:
			return SpritePivot::BottomRight;
		case 0:
		default:
			return SpritePivot::Center;
		}
	}

	template <typename T>
	static bool AddComponentItem(Registry &r, Entity e, const char *name)
	{
		if (r.Has<T>(e))
			return false;
		if (ImGui::MenuItem(name))
		{
			r.Emplace<T>(e);
			return true;
		}
		return false;
	}

	static void AddComponentPopup(Registry &r, Entity e, IEditorScene *editorScene)
	{
		if (!ImGui::BeginPopup("AddComponentPopup"))
			return;

		static char filter[128] = {};
		ImGui::InputText("Search", filter, sizeof(filter));

		auto pass = [&](const char *n)
		{
			if (filter[0] == 0)
				return true;
			return std::string(n).find(filter) != std::string::npos;
		};

		const bool is2DScene = editorScene && editorScene->GetEditorSceneDimension() == EditorSceneDimension::Scene2D;
		const bool allowSprite = is2DScene;
		const bool allowMeshAndMaterial = !is2DScene;

		if (pass("Transform"))
			(void)AddComponentItem<Transform>(r, e, "Transform");
		if (pass("Camera"))
			(void)AddComponentItem<Camera>(r, e, "Camera");
		if (pass("CameraController"))
			(void)AddComponentItem<CameraController>(r, e, "CameraController");
		if (pass("Spin"))
			(void)AddComponentItem<Spin>(r, e, "Spin");
		if (allowSprite && pass("Sprite"))
			(void)AddComponentItem<Sprite>(r, e, "Sprite");
		if (allowSprite && pass("SpriteAnimator"))
			(void)AddComponentItem<SpriteAnimator>(r, e, "SpriteAnimator");
		if (pass("Collider2D"))
			(void)AddComponentItem<Collider2D>(r, e, "Collider2D");
		if (pass("RigidBody2D"))
			(void)AddComponentItem<RigidBody2D>(r, e, "RigidBody2D");
		if (pass("PhysicsBody2D"))
			(void)AddComponentItem<PhysicsBody2D>(r, e, "PhysicsBody2D");
		if (allowMeshAndMaterial && pass("Material"))
			(void)AddComponentItem<Material>(r, e, "Material");
		if (allowMeshAndMaterial && pass("Mesh"))
			(void)AddComponentItem<Mesh>(r, e, "Mesh");
		if (pass("LightEmitter"))
			(void)AddComponentItem<LightEmitter>(r, e, "LightEmitter");

		ImGui::EndPopup();
	}

	template <typename T>
	static void RemoveComponentMenu(Registry &r, Entity e, const char *popupId)
	{
		if (!r.Has<T>(e))
			return;

		if (ImGui::BeginPopupContextItem(popupId))
		{
			if (ImGui::MenuItem("Remove"))
				r.Remove<T>(e);
			ImGui::EndPopup();
		}
	}

	void EditorSceneTools::SetDragSnapStep(float snapStep)
	{
		s_dragSnapStep = std::max(0.0001f, snapStep);
	}

	void EditorSceneTools::DrawInspector(Registry &r, SceneEditorState &st, IEditorScene *editorScene)
	{
		static AssetPickerRuntime s_assetPicker;
		Entity e = st.selectedEntity;

		if (e == kInvalidEntity || !r.IsAlive(e))
		{
			DrawAssetPickerPopup(s_assetPicker);
			ImGui::TextUnformatted("No entity selected.");
			return;
		}

		if (!r.Has<Tag>(e))
			r.Emplace<Tag>(e);

		if (!r.Has<Parent>(e))
			r.Emplace<Parent>(e, Parent{kInvalidEntity});

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponentPopup");

		AddComponentPopup(r, e, editorScene);

		ImGui::Separator();

		{
			auto &tag = r.Get<Tag>(e);
			(void)DrawBufferedStringInput(e, "Tag.Name", "Name", tag.name, 256);
		}

		{
			auto &p = r.Get<Parent>(e);
			unsigned pid = (p.parent == kInvalidEntity) ? 0u : (unsigned)p.parent;
			if (ImGui::InputScalar("Parent (id)", ImGuiDataType_U32, &pid))
			{
				const Entity requestedParent = (pid == 0u) ? kInvalidEntity : static_cast<Entity>(pid);
				if (requestedParent == kInvalidEntity)
				{
					ReparentEntityPreservingWorldTransform(r, e, kInvalidEntity);
				}
				else if (r.IsAlive(requestedParent) && requestedParent != e && !IsDescendant(r, requestedParent, e))
				{
					ReparentEntityPreservingWorldTransform(r, e, requestedParent);
				}
			}
		}

		ImGui::Separator();

		if (r.Has<Transform>(e) && ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<Transform>(r, e, "TransformCtx");

			auto &t = r.Get<Transform>(e);
			bool absolutePosition = t.absolutePosition;
			bool absoluteRotation = t.absoluteRotation;
			bool absoluteScale = t.absoluteScale;
			bool absoluteFlagsChanged = false;

			ImGui::TextUnformatted("Absolute");
			ImGui::SameLine();
			ImGui::PushID("TransformAbsoluteFlags");
			absoluteFlagsChanged |= DrawToggleButton("Position", absolutePosition);
			ImGui::SameLine();
			absoluteFlagsChanged |= DrawToggleButton("Rotation", absoluteRotation);
			ImGui::SameLine();
			absoluteFlagsChanged |= DrawToggleButton("Scale", absoluteScale);
			ImGui::PopID();

			if (absoluteFlagsChanged)
				ApplyAbsoluteFlagsPreservingWorld(r, e, absolutePosition, absoluteRotation, absoluteScale);

			DrawVec3(std::format("{} Position!", (absolutePosition ? "Abs." : "Local")).c_str(), &t.localPosition.x, 0.05f);
			DrawRotationEulerDegrees(std::format("{} Rotation!", (absoluteRotation ? "Abs." : "Local")).c_str(), t.localRotation, 0.5f);
			DrawVec3(std::format("{} Scale!", (absoluteScale ? "Abs." : "Local")).c_str(), &t.localScale.x, 0.05f);
			DrawVec3("Pivot Anchor (-1..1)", &t.pivot.x, 0.05f);
		}

		if (r.Has<Camera>(e) && ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<Camera>(r, e, "CameraCtx");

			auto &c = r.Get<Camera>(e);

			int proj = (c.projection == ProjectionType::Perspective) ? 0 : 1;
			if (ImGui::Combo("Projection", &proj, "Perspective\0Orthographic\0"))
				c.projection = (proj == 0) ? ProjectionType::Perspective : ProjectionType::Orthographic;

			DrawVec3("Position", &c.position.x, 0.05f);
			DrawVec3("Direction", &c.direction.x, 0.05f);

			(void)DragFloatWithSnap("FOV (deg)", &c.fovDeg, 0.1f, 1.f, 179.f);
			(void)DragFloatWithSnap("Near", &c.nearPlane, 0.001f, 0.0001f, 1000.f);
			(void)DragFloatWithSnap("Far", &c.farPlane, 0.1f, 0.1f, 100000.f);
			(void)DragFloatWithSnap("Ortho Height", &c.orthoHeight, 0.05f, 0.01f, 10000.f);
		}

		if (r.Has<CameraController>(e) && ImGui::CollapsingHeader("CameraController", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<CameraController>(r, e, "CameraControllerCtx");

			auto &cc = r.Get<CameraController>(e);
			(void)DragFloatWithSnap("Yaw (deg)", &cc.yawDeg, 0.1f);
			(void)DragFloatWithSnap("Pitch (deg)", &cc.pitchDeg, 0.1f);
			(void)DragFloatWithSnap("Move Speed", &cc.moveSpeed, 0.01f, 0.f, 1000.f);
			(void)DragFloatWithSnap("Fast Multiplier", &cc.fastMultiplier, 0.01f, 0.f, 1000.f);
			(void)DragFloatWithSnap("Mouse Sensitivity", &cc.mouseSensitivity, 0.001f, 0.f, 10.f);
			DrawVec2("Last Mouse Pos", &cc.lastMousePos.x, 0.05f);
			(void)DrawToggleButton("Has Last Mouse Pos", cc.hasLastMousePos);
		}

		if (r.Has<Spin>(e) && ImGui::CollapsingHeader("Spin", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<Spin>(r, e, "SpinCtx");

			auto &s = r.Get<Spin>(e);
			DrawVec3("Axis", &s.axis.x, 0.05f);
			(void)DragFloatWithSnap("Freq", &s.freq, 0.01f, 0.f, 1000.f);
			(void)DragFloatWithSnap("Amplitude", &s.amplitude, 0.01f, 0.f, 1000.f);
		}

		if (r.Has<LightEmitter>(e) && ImGui::CollapsingHeader("LightEmitter", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<LightEmitter>(r, e, "LightEmitterCtx");

			auto &l = r.Get<LightEmitter>(e);
			int type = static_cast<int>(l.type);
			if (ImGui::Combo("Type", &type, "Directional\0Point\0Spot\0"))
			{
				switch (type)
				{
				case 1:
					l.type = LightType::Point;
					break;
				case 2:
					l.type = LightType::Spot;
					break;
				case 0:
				default:
					l.type = LightType::Directional;
					break;
				}
			}

			ImGui::ColorEdit3("Color", &l.color.x);
			(void)DragFloatWithSnap("Intensity", &l.intensity, 0.01f, 0.0f, 1000.0f);

			if (l.type == LightType::Point || l.type == LightType::Spot)
				(void)DragFloatWithSnap("Range", &l.range, 0.05f, 0.0f, 10000.0f);

			if (l.type == LightType::Spot)
			{
				(void)DragFloatWithSnap("Inner Cone", &l.innerCone, 0.005f, 0.0f, 1.0f);
				(void)DragFloatWithSnap("Outer Cone", &l.outerCone, 0.005f, 0.0f, 1.0f);
			}

			(void)DrawToggleButton("Cast Shadows", l.castShadows);
		}

		if (r.Has<Sprite>(e) && ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<Sprite>(r, e, "SpriteCtx");

			auto &s = r.Get<Sprite>(e);
			if (DrawPathFieldWithAssetPicker(s_assetPicker, e, "Texture Path", "Sprite.TexturePath", AssetPickerType::Texture, s.texturePath))
			{
				std::string err;
				if (editorScene)
				{
					if (!editorScene->EditorSetSpriteTexture(e, s.texturePath, err))
						st.inspectorStatus = "Sprite texture error: " + err;
					else
						st.inspectorStatus.clear();
				}
			}

			if (DrawPathFieldWithAssetPicker(s_assetPicker, e, "Material Path", "Sprite.MaterialPath", AssetPickerType::Material, s.materialPath))
			{
				std::string err;
				if (editorScene)
				{
					if (!editorScene->EditorSetSpriteMaterial(e, s.materialPath, err))
						st.inspectorStatus = "Sprite material error: " + err;
					else
						st.inspectorStatus.clear();
				}
			}

			DrawVec2("Size", &s.size.x, 0.05f);
			int spritePivot = SpritePivotToIndex(s.pivot);
			if (ImGui::Combo("Sprite Pivot", &spritePivot,
							 "Center\0Top Left\0Top\0Top Right\0Left\0Right\0Bottom Left\0Bottom\0Bottom Right\0"))
			{
				s.pivot = SpritePivotFromIndex(spritePivot);
			}
			(void)DragFloat4WithSnap("UV", &s.uv.x, 0.01f);
			DrawColor4("Tint", &s.tint.x);
			ImGui::InputInt("Layer", &s.layer);
			ImGui::InputInt("Order In Layer", &s.orderInLayer);
		}

		if (r.Has<SpriteAnimator>(e) && ImGui::CollapsingHeader("SpriteAnimator", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<SpriteAnimator>(r, e, "SpriteAnimatorCtx");

			auto &a = r.Get<SpriteAnimator>(e);
			(void)DrawBufferedStringInput(e, "SpriteAnimator.ClipId", "Clip Id", a.clipId, 256);
			(void)DragFloatWithSnap("Time", &a.time, 0.01f);
			(void)DragFloatWithSnap("FPS", &a.fps, 0.05f, 0.0f, 1000.0f);
			(void)DragFloatWithSnap("Speed", &a.speed, 0.01f);
			ImGui::TextUnformatted("Playback");
			ImGui::SameLine();
			ImGui::PushID("SpriteAnimatorPlayback");
			(void)DrawToggleButton("Loop", a.loop);
			ImGui::SameLine();
			(void)DrawToggleButton("Playing", a.playing);
			ImGui::PopID();
			ImGui::InputInt("Current Frame", &a.currentFrame);
			a.currentFrame = std::max(0, a.currentFrame);
			ImGui::Separator();
			ImGui::TextUnformatted("Grid Fallback");
			ImGui::PushID("SpriteAnimatorGridFallback");
			(void)DrawInputIntPairHorizontal("Columns", &a.columns, "Rows", &a.rows);
			(void)DrawInputIntPairHorizontal("Gap X (px)", &a.gapX, "Gap Y (px)", &a.gapY);
			(void)DrawInputIntPairHorizontal("Margin X (px)", &a.marginX, "Margin Y (px)", &a.marginY);
			(void)DrawInputIntPairHorizontal("Start Index X", &a.startIndexX, "Start Index Y", &a.startIndexY);
			ImGui::InputInt("Frame Count (0=auto)", &a.frameCount);
			(void)DrawInputIntPairHorizontal("Cell Width (0=auto)", &a.cellWidth, "Cell Height (0=auto)", &a.cellHeight);
			ImGui::PopID();
			a.columns = std::max(1, a.columns);
			a.rows = std::max(1, a.rows);
			a.fps = std::max(0.0f, a.fps);
			a.gapX = std::max(0, a.gapX);
			a.gapY = std::max(0, a.gapY);
			a.marginX = std::max(0, a.marginX);
			a.marginY = std::max(0, a.marginY);
			a.startIndexX = std::max(0, a.startIndexX);
			a.startIndexY = std::max(0, a.startIndexY);
			a.frameCount = std::max(0, a.frameCount);
			a.cellWidth = std::max(0, a.cellWidth);
			a.cellHeight = std::max(0, a.cellHeight);
		}

		if (r.Has<Collider2D>(e) && ImGui::CollapsingHeader("Collider2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<Collider2D>(r, e, "Collider2DCtx");

			auto &c = r.Get<Collider2D>(e);
			int shapeIndex = (c.shape == Collider2D::Shape::Circle) ? 1 : 0;
			if (ImGui::Combo("Shape", &shapeIndex, "AABB\0Circle\0"))
				c.shape = (shapeIndex == 1) ? Collider2D::Shape::Circle : Collider2D::Shape::AABB;

			(void)DrawToggleButton("Is Trigger", c.isTrigger);
			ImGui::InputScalar("Layer", ImGuiDataType_U32, &c.layer);
			ImGui::InputScalar("Mask", ImGuiDataType_U32, &c.mask);
			DrawVec2("Offset", &c.offset.x, 0.05f);

			if (c.shape == Collider2D::Shape::AABB)
			{
				DrawVec2("Half Extents", &c.halfExtents.x, 0.05f);
				c.halfExtents.x = std::max(0.f, c.halfExtents.x);
				c.halfExtents.y = std::max(0.f, c.halfExtents.y);
			}
			else
			{
				(void)DragFloatWithSnap("Radius", &c.radius, 0.01f, 0.0f, 1000.0f);
			}
		}

		if (r.Has<RigidBody2D>(e) && ImGui::CollapsingHeader("RigidBody2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<RigidBody2D>(r, e, "RigidBody2DCtx");

			auto &rb = r.Get<RigidBody2D>(e);
			DrawVec3("Velocity", &rb.velocity.x, 0.05f);
			DrawVec3("Accumulated Forces", &rb.accumulatedForces.x, 0.05f);
			DrawVec3("Angular Velocity", &rb.angularVelocity.x, 0.05f);
			(void)DrawToggleButton("Is Kinematic", rb.isKinematic);

			ImGui::TextUnformatted("Freeze Velocity");
			ImGui::SameLine();
			ImGui::PushID("FreezeVelocity");
			(void)DrawToggleButton("X", rb.freezeVelocityX);
			ImGui::SameLine();
			(void)DrawToggleButton("Y", rb.freezeVelocityY);
			ImGui::SameLine();
			(void)DrawToggleButton("Z", rb.freezeVelocityZ);
			ImGui::PopID();

			ImGui::TextUnformatted("Freeze Angular");
			ImGui::SameLine();
			ImGui::PushID("FreezeAngularVelocity");
			(void)DrawToggleButton("X", rb.freezeAngularVelocityX);
			ImGui::SameLine();
			(void)DrawToggleButton("Y", rb.freezeAngularVelocityY);
			ImGui::SameLine();
			(void)DrawToggleButton("Z", rb.freezeAngularVelocityZ);
			ImGui::PopID();

			(void)DragFloatWithSnap("Mass", &rb.mass, 0.01f, 0.0f, 100000.0f);
			(void)DragFloatWithSnap("Linear Damping", &rb.linearDamping, 0.001f, 0.0f, 1000.0f);
			(void)DragFloatWithSnap("Angular Damping", &rb.angularDamping, 0.001f, 0.0f, 1000.0f);
		}

		if (r.Has<PhysicsBody2D>(e) && ImGui::CollapsingHeader("PhysicsBody2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<PhysicsBody2D>(r, e, "PhysicsBody2DCtx");
			auto &pb = r.Get<PhysicsBody2D>(e);
			(void)DrawToggleButton("Enabled", pb.enabled);
		}

		if (r.Has<Material>(e) && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<Material>(r, e, "MaterialCtx");

			auto &m = r.Get<Material>(e);
			std::string shaderPath = m.shaderPath;
			if (shaderPath.empty() && r.Has<Mesh>(e))
				shaderPath = r.Get<Mesh>(e).materialPath;

			const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(shaderPath);
			SyncMaterialParametersWithMetadata(m, metadata);

			bool materialPathChanged = false;
			std::string currentGroup;

			for (const MaterialParameterMetadata &desc : metadata.parameters)
			{
				auto paramIt = m.parameters.find(desc.name);
				if (paramIt == m.parameters.end())
					continue;

				MaterialParameter &param = paramIt->second;

				if (!desc.uiGroup.empty() && desc.uiGroup != currentGroup)
				{
					currentGroup = desc.uiGroup;
					ImGui::Separator();
					ImGui::TextUnformatted(currentGroup.c_str());
				}

				switch (desc.type)
				{
				case MaterialParameterType::Float:
					if (desc.hasRange)
						(void)DragFloatWithSnap(desc.name.c_str(), &param.numericValue.x, 0.01f, desc.rangeMin, desc.rangeMax);
					else
						(void)DragFloatWithSnap(desc.name.c_str(), &param.numericValue.x, 0.01f);
					break;
				case MaterialParameterType::Vec2:
					(void)DragFloat2WithSnap(desc.name.c_str(), &param.numericValue.x, 0.01f);
					if (desc.hasRange)
					{
						param.numericValue.x = std::clamp(param.numericValue.x, desc.rangeMin, desc.rangeMax);
						param.numericValue.y = std::clamp(param.numericValue.y, desc.rangeMin, desc.rangeMax);
					}
					break;
				case MaterialParameterType::Vec3:
					if (desc.hasRange)
						(void)DragFloat3WithSnap(desc.name.c_str(), &param.numericValue.x, 0.01f, desc.rangeMin, desc.rangeMax);
					else
						(void)DragFloat3WithSnap(desc.name.c_str(), &param.numericValue.x, 0.01f);
					break;
				case MaterialParameterType::Vec4:
					(void)DragFloat4WithSnap(desc.name.c_str(), &param.numericValue.x, 0.01f);
					if (desc.hasRange)
					{
						param.numericValue.x = std::clamp(param.numericValue.x, desc.rangeMin, desc.rangeMax);
						param.numericValue.y = std::clamp(param.numericValue.y, desc.rangeMin, desc.rangeMax);
						param.numericValue.z = std::clamp(param.numericValue.z, desc.rangeMin, desc.rangeMax);
						param.numericValue.w = std::clamp(param.numericValue.w, desc.rangeMin, desc.rangeMax);
					}
					break;
				case MaterialParameterType::Texture2D:
				{
					const std::string key = "Material.ParamPath." + desc.name;
					materialPathChanged |= DrawPathFieldWithAssetPicker(s_assetPicker,
																		e,
																		desc.name.c_str(),
																		key.c_str(),
																		AssetPickerType::Texture,
																		param.texturePath);
					break;
				}
				default:
					break;
				}

				if (!desc.tooltip.empty() && ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", desc.tooltip.c_str());
			}

			if (metadata.Empty())
				ImGui::TextUnformatted("No shader material metadata found.");

			if ((materialPathChanged || ImGui::Button("Apply Material Parameters")) && editorScene)
			{
				std::string err;
				if (!editorScene->EditorApplyMaterial(e, err))
					st.inspectorStatus = "Material apply error: " + err;
				else
					st.inspectorStatus.clear();
			}
		}

		if (r.Has<Mesh>(e) && ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RemoveComponentMenu<Mesh>(r, e, "MeshCtx");

			auto &m = r.Get<Mesh>(e);
			if (DrawPathFieldWithAssetPicker(s_assetPicker, e, "Mesh Path", "Mesh.MeshPath", AssetPickerType::Mesh, m.meshPath))
			{
				std::string err;
				if (editorScene)
				{
					if (!editorScene->EditorSetMeshPath(e, m.meshPath, err))
						st.inspectorStatus = "Mesh path error: " + err;
					else
						st.inspectorStatus.clear();
				}
			}

			if (DrawPathFieldWithAssetPicker(s_assetPicker, e, "Material Path", "Mesh.MaterialPath", AssetPickerType::Material, m.materialPath))
			{
				std::string err;
				if (editorScene)
				{
					if (!editorScene->EditorSetMeshMaterial(e, m.materialPath, err))
						st.inspectorStatus = "Mesh material error: " + err;
					else
						st.inspectorStatus.clear();
				}
			}

			ImGui::Text("Index Count: %u", m.gpuIndexCount);
		}

		if (!st.inspectorStatus.empty())
			ImGui::TextWrapped("%s", st.inspectorStatus.c_str());

		DrawAssetPickerPopup(s_assetPicker);
	}
	void HierarchyPanel::Draw(Registry &registry, SceneEditorState &state)
	{
		EditorSceneTools::DrawHierarchy(registry, state);
	}

	void InspectorPanel::Draw(Registry &registry, SceneEditorState &state, IEditorScene *editorScene)
	{
		EditorSceneTools::DrawInspector(registry, state, editorScene);
	}

	void EditorSceneToolsFacade::DrawHierarchy(Registry &registry, SceneEditorState &state)
	{
		EditorSceneTools::DrawHierarchy(registry, state);
	}

	void EditorSceneToolsFacade::DrawInspector(Registry &registry, SceneEditorState &state, IEditorScene *editorScene)
	{
		EditorSceneTools::DrawInspector(registry, state, editorScene);
	}

	void EditorSceneToolsFacade::SetDragSnapStep(float snapStep)
	{
		EditorSceneTools::SetDragSnapStep(snapStep);
	}

	Entity EditorSceneToolsFacade::CreateEntity(Registry &registry, const char *name, Entity parent, bool addTransform)
	{
		return EditorSceneTools::CreateEntity(registry, name, parent, addTransform);
	}

	void EditorSceneToolsFacade::DestroyEntityRecursive(Registry &registry, Entity e)
	{
		EditorSceneTools::DestroyEntityRecursive(registry, e);
	}

	Entity EditorSceneToolsFacade::DuplicateEntityRecursive(Registry &registry, Entity src)
	{
		return EditorSceneTools::DuplicateEntityRecursive(registry, src);
	}

	void EditorSceneToolsFacade::BeginRename(Registry &registry, SceneEditorState &state, Entity e)
	{
		EditorSceneTools::BeginRename(registry, state, e);
	}
}
#pragma endregion
