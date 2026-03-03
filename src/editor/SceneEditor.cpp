#pragma region Includes
#include "SceneEditor.h"

#include <imgui.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>

#include "../components/Tag.h"
#include "../components/Parent.h"

#include "../components/Transform.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Sprite.h"
#include "../components/Spin.h"
#pragma endregion

#pragma region Function Definitions
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

static void DrawVec2(const char *label, float *v, float speed)
{
	ImGui::DragFloat2(label, v, speed);
}

static void DrawColor4(const char *label, float *v)
{
	ImGui::ColorEdit4(label, v);
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

static void AddComponentPopup(Registry &r, Entity e)
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

	if (pass("Transform"))
		(void)AddComponentItem<Transform>(r, e, "Transform");
	if (pass("Camera"))
		(void)AddComponentItem<Camera>(r, e, "Camera");
	if (pass("CameraController"))
		(void)AddComponentItem<CameraController>(r, e, "CameraController");
	if (pass("Spin"))
		(void)AddComponentItem<Spin>(r, e, "Spin");
	if (pass("Sprite"))
		(void)AddComponentItem<Sprite>(r, e, "Sprite");
	if (pass("Material"))
		(void)AddComponentItem<Material>(r, e, "Material");
	if (pass("Mesh"))
		(void)AddComponentItem<Mesh>(r, e, "Mesh");

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

void SceneEditor::DrawInspector(Registry &r, SceneEditorState &st)
{
	Entity e = st.selectedEntity;

	if (e == kInvalidEntity || !r.IsAlive(e))
	{
		ImGui::TextUnformatted("No entity selected.");
		return;
	}

	if (!r.Has<Tag>(e))
		r.Emplace<Tag>(e);

	if (!r.Has<Parent>(e))
		r.Emplace<Parent>(e, Parent{kInvalidEntity});

	if (ImGui::Button("Add Component"))
		ImGui::OpenPopup("AddComponentPopup");

	AddComponentPopup(r, e);

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
		DrawVec3("RotationEuler", &t.rotationEuler.x, 0.05f);
		DrawVec3("Scale", &t.scale.x, 0.05f);
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

	if (r.Has<Sprite>(e) && ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RemoveComponentMenu<Sprite>(r, e, "SpriteCtx");

		auto &s = r.Get<Sprite>(e);

		unsigned texId = s.texture.id;
		unsigned shId = s.shader.id;

		ImGui::InputScalar("Texture Handle", ImGuiDataType_U32, &texId);
		ImGui::InputScalar("Shader Handle", ImGuiDataType_U32, &shId);

		s.texture.id = texId;
		s.shader.id = shId;

		DrawVec2("Size", &s.size.x, 0.05f);
		ImGui::DragFloat4("UV", &s.uv.x, 0.01f);
		DrawColor4("Tint", &s.tint.x);
		ImGui::InputInt("Layer", &s.layer);
		ImGui::InputInt("Order In Layer", &s.orderInLayer);
	}

	if (r.Has<Material>(e) && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RemoveComponentMenu<Material>(r, e, "MaterialCtx");

		auto &m = r.Get<Material>(e);

		unsigned shId = m.shader.id;
		unsigned txId = m.texture.id;

		ImGui::InputScalar("Shader Handle", ImGuiDataType_U32, &shId);
		ImGui::InputScalar("Texture Handle", ImGuiDataType_U32, &txId);

		m.shader.id = shId;
		m.texture.id = txId;

		DrawColor4("Tint", &m.tint.x);
	}

	if (r.Has<Mesh>(e) && ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RemoveComponentMenu<Mesh>(r, e, "MeshCtx");

		auto &m = r.Get<Mesh>(e);

		unsigned vao = m.vao;
		unsigned vbo = m.vbo;
		unsigned ebo = m.ebo;

		ImGui::InputScalar("VAO", ImGuiDataType_U32, &vao);
		ImGui::InputScalar("VBO", ImGuiDataType_U32, &vbo);
		ImGui::InputScalar("EBO", ImGuiDataType_U32, &ebo);

		ImGui::InputInt("Index Count", (int *)&m.indexCount);
	}
}
#pragma endregion
