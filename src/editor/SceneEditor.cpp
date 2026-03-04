#pragma region Includes
#include "SceneEditor.h"

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
#pragma endregion

#pragma region Function Definitions
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

	char pathBuf[512];
	std::snprintf(pathBuf, sizeof(pathBuf), "%s", inOutValue.c_str());

	ImGui::PushID(fieldKey);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::SameLine();

	const float buttonWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
	const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	const float avail = ImGui::GetContentRegionAvail().x;
	const float inputWidth = std::max(80.0f, avail - buttonWidth - spacing);
	ImGui::SetNextItemWidth(inputWidth);

	if (ImGui::InputText("##Path", pathBuf, sizeof(pathBuf)))
	{
		inOutValue = pathBuf;
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
	std::snprintf(tmp, (size_t)tmpSize, "Entity %u", (unsigned)e);
	return tmp;
}

static Entity GetParentOf(Registry &r, Entity e)
{
	if (!r.Has<Parent>(e))
		return kInvalidEntity;
	return r.Get<Parent>(e).parent;
}

static void SetParentOf(Registry &r, Entity e, Entity parent)
{
	if (!r.Has<Parent>(e))
		r.Emplace<Parent>(e, Parent{parent});
	else
		r.Get<Parent>(e).parent = parent;
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

Entity SceneEditor::CreateEntity(Registry &r, const char *name, Entity parent, bool addTransform)
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

void SceneEditor::DestroyEntityRecursive(Registry &r, Entity e)
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
	CopyIfPresent<Spin>(r, src, dst);
	CopyIfPresent<LightEmitter>(r, src, dst);

	auto it = children.find(src);
	if (it != children.end())
	{
		for (Entity c : it->second)
			(void)DuplicateRecurse(r, c, dst, children);
	}

	return dst;
}

Entity SceneEditor::DuplicateEntityRecursive(Registry &r, Entity src)
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

void SceneEditor::BeginRename(Registry &r, SceneEditorState &st, Entity e)
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
			SceneEditor::BeginRename(r, st, e);

		if (ImGui::BeginPopupContextItem("ctx"))
		{
			if (ImGui::MenuItem("Create Child"))
				st.selectedEntity = SceneEditor::CreateEntity(r, "New Entity", e, true);

			if (ImGui::MenuItem("Duplicate"))
				st.selectedEntity = SceneEditor::DuplicateEntityRecursive(r, e);

			if (ImGui::MenuItem("Rename"))
				SceneEditor::BeginRename(r, st, e);

			if (ImGui::MenuItem("Delete"))
			{
				SceneEditor::DestroyEntityRecursive(r, e);
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
					SetParentOf(r, dropped, e);
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

void SceneEditor::DrawHierarchy(Registry &r, SceneEditorState &st)
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
			SetParentOf(r, dropped, kInvalidEntity);
		}
		ImGui::EndDragDropTarget();
	}

	for (Entity e : roots)
		DrawEntityNode(r, e, children, st);
}

static void DrawVec3(const char *label, float *v, float speed)
{
	ImGui::DragFloat3(label, v, speed);
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

	if (ImGui::DragFloat3(label,
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
	ImGui::DragFloat2(label, v, speed);
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

void SceneEditor::DrawInspector(Registry &r, SceneEditorState &st, IEditorScene *editorScene)
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
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s", tag.name.c_str());
		if (ImGui::InputText("Name", buf, sizeof(buf)))
			tag.name = buf;
	}

	{
		auto &p = r.Get<Parent>(e);
		unsigned pid = (p.parent == kInvalidEntity) ? 0u : (unsigned)p.parent;
		ImGui::InputScalar("Parent (id)", ImGuiDataType_U32, &pid);
	}

	ImGui::Separator();

	if (r.Has<Transform>(e) && ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RemoveComponentMenu<Transform>(r, e, "TransformCtx");

		auto &t = r.Get<Transform>(e);
		DrawVec3("Position", &t.position.x, 0.05f);
		DrawRotationEulerDegrees("RotationEuler", t.rotationEuler, 0.5f);
		DrawVec3("Scale", &t.scale.x, 0.05f);
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

		ImGui::DragFloat("FOV (deg)", &c.fovDeg, 0.1f, 1.f, 179.f);
		ImGui::DragFloat("Near", &c.nearPlane, 0.001f, 0.0001f, 1000.f);
		ImGui::DragFloat("Far", &c.farPlane, 0.1f, 0.1f, 100000.f);
		ImGui::DragFloat("Ortho Height", &c.orthoHeight, 0.05f, 0.01f, 10000.f);
	}

	if (r.Has<CameraController>(e) && ImGui::CollapsingHeader("CameraController", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RemoveComponentMenu<CameraController>(r, e, "CameraControllerCtx");

		auto &cc = r.Get<CameraController>(e);
		ImGui::DragFloat("Yaw (deg)", &cc.yawDeg, 0.1f);
		ImGui::DragFloat("Pitch (deg)", &cc.pitchDeg, 0.1f);
		ImGui::DragFloat("Move Speed", &cc.moveSpeed, 0.01f, 0.f, 1000.f);
		ImGui::DragFloat("Fast Multiplier", &cc.fastMultiplier, 0.01f, 0.f, 1000.f);
		ImGui::DragFloat("Mouse Sensitivity", &cc.mouseSensitivity, 0.001f, 0.f, 10.f);
		DrawVec2("Last Mouse Pos", &cc.lastMousePos.x, 0.05f);
		ImGui::Checkbox("Has Last Mouse Pos", &cc.hasLastMousePos);
	}

	if (r.Has<Spin>(e) && ImGui::CollapsingHeader("Spin", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RemoveComponentMenu<Spin>(r, e, "SpinCtx");

		auto &s = r.Get<Spin>(e);
		DrawVec3("Axis", &s.axis.x, 0.05f);
		ImGui::DragFloat("Freq", &s.freq, 0.01f, 0.f, 1000.f);
		ImGui::DragFloat("Amplitude", &s.amplitude, 0.01f, 0.f, 1000.f);
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
		ImGui::DragFloat("Intensity", &l.intensity, 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat("Range", &l.range, 0.05f, 0.0f, 10000.0f);
		ImGui::DragFloat("Inner Cone", &l.innerCone, 0.005f, 0.0f, 1.0f);
		ImGui::DragFloat("Outer Cone", &l.outerCone, 0.005f, 0.0f, 1.0f);
		ImGui::Checkbox("Cast Shadows", &l.castShadows);
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
		ImGui::DragFloat4("UV", &s.uv.x, 0.01f);
		DrawColor4("Tint", &s.tint.x);
		ImGui::InputInt("Layer", &s.layer);
		ImGui::InputInt("Order In Layer", &s.orderInLayer);
	}

	if (r.Has<Material>(e) && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RemoveComponentMenu<Material>(r, e, "MaterialCtx");

		auto &m = r.Get<Material>(e);
		bool materialPathChanged = false;
		materialPathChanged |= DrawPathFieldWithAssetPicker(s_assetPicker, e, "Base Color Path", "Material.TexturePath", AssetPickerType::Texture, m.texturePath);
		materialPathChanged |= DrawPathFieldWithAssetPicker(s_assetPicker, e, "Specular Path", "Material.SpecularTexturePath", AssetPickerType::Texture, m.specularTexturePath);
		materialPathChanged |= DrawPathFieldWithAssetPicker(s_assetPicker, e, "Normal Path", "Material.NormalTexturePath", AssetPickerType::Texture, m.normalTexturePath);
		materialPathChanged |= DrawPathFieldWithAssetPicker(s_assetPicker, e, "Emission Path", "Material.EmissionTexturePath", AssetPickerType::Texture, m.emissionTexturePath);

		DrawColor4("Tint", &m.tint.x);
		ImGui::DragFloat("Specular Strength", &m.specularStrength, 0.01f, 0.0f, 8.0f);
		ImGui::DragFloat("Shininess", &m.shininess, 0.1f, 1.0f, 512.0f);
		ImGui::DragFloat("Normal Strength", &m.normalStrength, 0.01f, 0.0f, 4.0f);
		ImGui::DragFloat("Emission Strength", &m.emissionStrength, 0.01f, 0.0f, 16.0f);

		if ((materialPathChanged || ImGui::Button("Apply Material Textures")) && editorScene)
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

		ImGui::Text("Index Count: %d", (int)m.indexCount);
	}

	if (!st.inspectorStatus.empty())
		ImGui::TextWrapped("%s", st.inspectorStatus.c_str());

	DrawAssetPickerPopup(s_assetPicker);
}
#pragma endregion
