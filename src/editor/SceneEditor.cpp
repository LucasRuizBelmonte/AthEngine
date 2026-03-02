#include "SceneEditor.h"

#include <imgui.h>

#include "../components/Tag.h"
#include "../components/Parent.h"

#include "../components/Transform.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Sprite.h"
#include "../components/Spin.h"

#include <glm/glm.hpp>

static const char *GetEntityLabel(Registry &registry, Entity e, char *buf, int bufSize)
{
	if (registry.Has<Tag>(e))
	{
		const auto &t = registry.Get<Tag>(e);
		if (!t.name.empty())
			return t.name.c_str();
	}

	snprintf(buf, (size_t)bufSize, "Entity %u", (unsigned)e);
	return buf;
}

void SceneEditor::BuildChildrenMap(
	Registry &registry,
	std::unordered_map<Entity, std::vector<Entity>> &outChildren,
	std::vector<Entity> &outRoots)
{
	outChildren.clear();
	outRoots.clear();

	const auto &alive = registry.Alive();
	outRoots.reserve(alive.size());

	for (Entity e : alive)
	{
		Entity p = kInvalidEntity;
		if (registry.Has<Parent>(e))
			p = registry.Get<Parent>(e).parent;

		if (p == kInvalidEntity)
		{
			outRoots.push_back(e);
		}
		else
		{
			outChildren[p].push_back(e);
		}
	}
}

void SceneEditor::DrawEntityNode(
	Registry &registry,
	Entity e,
	const std::unordered_map<Entity, std::vector<Entity>> &children,
	Entity &selectedEntity)
{
	auto it = children.find(e);
	const bool hasChildren = (it != children.end() && !it->second.empty());

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (!hasChildren)
		flags |= ImGuiTreeNodeFlags_Leaf;

	if (selectedEntity == e)
		flags |= ImGuiTreeNodeFlags_Selected;

	char tmp[64];
	const char *label = GetEntityLabel(registry, e, tmp, 64);

	ImGui::PushID((int)e);
	const bool open = ImGui::TreeNodeEx("##node", flags, "%s", label);

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		selectedEntity = e;

	if (open)
	{
		if (hasChildren)
		{
			for (Entity c : it->second)
				DrawEntityNode(registry, c, children, selectedEntity);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void SceneEditor::DrawEntityHierarchy(Registry &registry, Entity &selectedEntity)
{
	std::unordered_map<Entity, std::vector<Entity>> children;
	std::vector<Entity> roots;
	BuildChildrenMap(registry, children, roots);

	for (Entity r : roots)
		DrawEntityNode(registry, r, children, selectedEntity);
}

static void DrawVec3(const char *label, glm::vec3 &v, float speed = 0.05f)
{
	ImGui::DragFloat3(label, &v.x, speed);
}

static void DrawVec2(const char *label, glm::vec2 &v, float speed = 0.05f)
{
	ImGui::DragFloat2(label, &v.x, speed);
}

static void DrawVec4(const char *label, glm::vec4 &v)
{
	ImGui::ColorEdit4(label, &v.x);
}

void SceneEditor::DrawInspector(Registry &registry, Entity selectedEntity)
{
	if (selectedEntity == kInvalidEntity)
	{
		ImGui::TextUnformatted("No entity selected.");
		return;
	}

	bool alive = false;
	for (Entity e : registry.Alive())
	{
		if (e == selectedEntity)
		{
			alive = true;
			break;
		}
	}

	if (!alive)
	{
		ImGui::TextUnformatted("Selected entity is not alive.");
		return;
	}

	if (registry.Has<Tag>(selectedEntity))
	{
		auto &tag = registry.Get<Tag>(selectedEntity);
		char buf[256];
		snprintf(buf, 256, "%s", tag.name.c_str());
		if (ImGui::InputText("Name", buf, 256))
			tag.name = buf;
	}
	else
	{
		ImGui::TextUnformatted("Tag: <missing>");
	}

	if (registry.Has<Parent>(selectedEntity))
	{
		auto &p = registry.Get<Parent>(selectedEntity);
		unsigned int pid = (p.parent == kInvalidEntity) ? 0u : (unsigned)p.parent;
		ImGui::InputScalar("Parent (id)", ImGuiDataType_U32, &pid);
	}
	else
	{
		ImGui::TextUnformatted("Parent: <missing>");
	}

	ImGui::Separator();

	if (registry.Has<Transform>(selectedEntity) && ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto &t = registry.Get<Transform>(selectedEntity);
		DrawVec3("Position", t.position);
		DrawVec3("RotationEuler", t.rotationEuler);
		DrawVec3("Scale", t.scale);
	}

	if (registry.Has<Camera>(selectedEntity) && ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto &c = registry.Get<Camera>(selectedEntity);

		int proj = (c.projection == ProjectionType::Perspective) ? 0 : 1;
		if (ImGui::Combo("Projection", &proj, "Perspective\0Orthographic\0"))
			c.projection = (proj == 0) ? ProjectionType::Perspective : ProjectionType::Orthographic;

		DrawVec3("Position", c.position);
		DrawVec3("Direction", c.direction);

		ImGui::DragFloat("FOV (deg)", &c.fovDeg, 0.1f, 1.f, 179.f);
		ImGui::DragFloat("Near", &c.nearPlane, 0.001f, 0.0001f, 1000.f);
		ImGui::DragFloat("Far", &c.farPlane, 0.1f, 0.1f, 100000.f);
		ImGui::DragFloat("Ortho Height", &c.orthoHeight, 0.05f, 0.01f, 10000.f);
	}

	if (registry.Has<CameraController>(selectedEntity) && ImGui::CollapsingHeader("CameraController", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto &cc = registry.Get<CameraController>(selectedEntity);
		ImGui::DragFloat("Yaw (deg)", &cc.yawDeg, 0.1f);
		ImGui::DragFloat("Pitch (deg)", &cc.pitchDeg, 0.1f);
		ImGui::DragFloat("Move Speed", &cc.moveSpeed, 0.01f, 0.f, 1000.f);
		ImGui::DragFloat("Fast Multiplier", &cc.fastMultiplier, 0.01f, 0.f, 1000.f);
		ImGui::DragFloat("Mouse Sensitivity", &cc.mouseSensitivity, 0.001f, 0.f, 10.f);
		DrawVec2("Last Mouse Pos", cc.lastMousePos);
		ImGui::Checkbox("Has Last Mouse Pos", &cc.hasLastMousePos);
	}

	if (registry.Has<Spin>(selectedEntity) && ImGui::CollapsingHeader("Spin", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto &s = registry.Get<Spin>(selectedEntity);
		DrawVec3("Axis", s.axis);
		ImGui::DragFloat("Freq", &s.freq, 0.01f, 0.f, 1000.f);
		ImGui::DragFloat("Amplitude", &s.amplitude, 0.01f, 0.f, 1000.f);
	}

	if (registry.Has<Sprite>(selectedEntity) && ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto &s = registry.Get<Sprite>(selectedEntity);
		unsigned int texId = s.texture.id;
		unsigned int shId = s.shader.id;

		ImGui::InputScalar("Texture Handle", ImGuiDataType_U32, &texId);
		ImGui::InputScalar("Shader Handle", ImGuiDataType_U32, &shId);

		s.texture.id = texId;
		s.shader.id = shId;

		DrawVec2("Size", s.size);
		ImGui::DragFloat4("UV", &s.uv.x, 0.01f);
		DrawVec4("Tint", s.tint);
		ImGui::InputInt("Layer", &s.layer);
		ImGui::InputInt("Order In Layer", &s.orderInLayer);
	}

	if (registry.Has<Material>(selectedEntity) && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto &m = registry.Get<Material>(selectedEntity);
		unsigned int shId = m.shader.id;
		unsigned int txId = m.texture.id;

		ImGui::InputScalar("Shader Handle", ImGuiDataType_U32, &shId);
		ImGui::InputScalar("Texture Handle", ImGuiDataType_U32, &txId);

		m.shader.id = shId;
		m.texture.id = txId;

		DrawVec4("Tint", m.tint);
	}

	if (registry.Has<Mesh>(selectedEntity) && ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto &m = registry.Get<Mesh>(selectedEntity);
		unsigned int vao = m.vao;
		unsigned int vbo = m.vbo;
		unsigned int ebo = m.ebo;

		ImGui::InputScalar("VAO", ImGuiDataType_U32, &vao);
		ImGui::InputScalar("VBO", ImGuiDataType_U32, &vbo);
		ImGui::InputScalar("EBO", ImGuiDataType_U32, &ebo);

		ImGui::InputInt("Index Count", (int *)&m.indexCount);
	}
}