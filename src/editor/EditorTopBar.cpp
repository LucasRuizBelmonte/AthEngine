#include "EditorModulesInternal.h"

#include "EditorUI.h"
#include "SceneEditor.h"

#include "../components/Material.h"
#include "../components/Mesh.h"
#include "../components/Transform.h"
#include "../prefab/PrefabRegistry.h"
#include "../scene/IEditorScene.h"
#include "../scene/SceneManager.h"
#include "../utils/Console.h"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace editorui::internal
{
	namespace
	{
		bool IsEditable3DScene(IEditorScene *editorScene)
		{
			return editorScene && editorScene->GetEditorSceneDimension() == EditorSceneDimension::Scene3D;
		}

		Entity AddBasicShape(IEditorScene *editorScene, SceneEditorState &se, const std::string &name, const std::string &meshPath)
		{
			if (!editorScene)
				return kInvalidEntity;

			Registry &registry = editorScene->GetEditorRegistry();
			Entity entity = SceneEditor::CreateEntity(registry, name.c_str(), kInvalidEntity, true);

			if (!registry.Has<Mesh>(entity))
				registry.Emplace<Mesh>(entity);

			auto &mesh = registry.Get<Mesh>(entity);
			mesh.meshPath = meshPath;
			mesh.materialPath = "res/shaders/lit3D.fs";

			if (!registry.Has<Material>(entity))
				registry.Emplace<Material>(entity);

			std::string err;
			if (!editorScene->EditorSetMeshPath(entity, mesh.meshPath, err))
			{
				se.inspectorStatus = "Basic shape mesh error: " + err;
			}

			if (!editorScene->EditorSetMeshMaterial(entity, mesh.materialPath, err))
				se.inspectorStatus = "Basic shape material error: " + err;
			else if (se.inspectorStatus.rfind("Basic shape mesh error:", 0) != 0)
				se.inspectorStatus.clear();

			return entity;
		}

		Transform BuildPrefabSpawnTransform(IEditorScene *editorScene, const SceneEditorState &se)
		{
			Transform at;
			if (!editorScene)
				return at;

			Registry &registry = editorScene->GetEditorRegistry();
			if (se.selectedEntity != kInvalidEntity &&
				registry.IsAlive(se.selectedEntity) &&
				registry.Has<Transform>(se.selectedEntity))
			{
				at = registry.Get<Transform>(se.selectedEntity);
			}

			return at;
		}

		Entity AddPrefab(IEditorScene *editorScene,
		                 SceneEditorState &se,
		                 const std::string &prefabName,
		                 const prefab::PrefabSpawnOverrides *overrides = nullptr)
		{
			if (!editorScene)
				return kInvalidEntity;

			const Transform at = BuildPrefabSpawnTransform(editorScene, se);
			const Entity entity = overrides
			                          ? editorScene->SpawnPrefab(prefabName, at, *overrides)
			                          : editorScene->SpawnPrefab(prefabName, at);

			if (entity == kInvalidEntity)
				se.inspectorStatus = "Prefab spawn failed: " + prefabName;
			else
				se.inspectorStatus.clear();

			return entity;
		}

		void AddSystemPopup(IEditorScene *editorScene)
		{
			if (!editorScene)
				return;

			if (!ImGui::BeginPopup("AddSystemPopup"))
				return;

			static char filter[128] = {};
			ImGui::InputText("Search", filter, sizeof(filter));

			const bool hasFilter = (filter[0] != 0);
			auto toLowerCopy = [](std::string value)
			{
				std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
							   { return static_cast<char>(std::tolower(c)); });
				return value;
			};
			const std::string filterLower = toLowerCopy(filter);
			auto folderPathForSystem = [&](const char *name)
			{
				const std::string lower = toLowerCopy(name ? name : "");
				if (lower.find("camera") != std::string::npos)
					return "Camera/Control";
				if (lower.find("render") != std::string::npos)
					return (lower.find("2d") != std::string::npos) ? "Rendering/2D" : "Rendering/3D";
				if (lower.find("animation") != std::string::npos || lower.find("sprite") != std::string::npos)
					return "Animation/2D";
				if (lower.find("physics") != std::string::npos || lower.find("trigger") != std::string::npos)
					return "Physics/2D";
				if (lower.find("ui") != std::string::npos)
					return "UI/Core";
				return "Gameplay/Core";
			};
			auto pass = [&](const std::string &path)
			{
				if (!hasFilter)
					return true;
				return toLowerCopy(path).find(filterLower) != std::string::npos;
			};

			std::vector<EditorSystemToggle> systems;
			editorScene->GetEditorSystems(systems);
			auto &removedSystems = RemovedSystemsByScene()[editorScene];
			auto addSystem = [&](EditorSystemToggle &sys, const char *label)
			{
				if (ImGui::MenuItem(label))
				{
					if (sys.enabled)
						*sys.enabled = true;
					removedSystems.erase(sys.name);
				}
			};

			bool anyAddable = false;
			if (hasFilter)
			{
				for (auto &sys : systems)
				{
					const bool isRemoved = removedSystems.find(sys.name) != removedSystems.end();
					const bool isEnabled = (sys.enabled && *sys.enabled);
					if (!isRemoved && isEnabled)
						continue;

					const std::string path = std::string(folderPathForSystem(sys.name)) + "/" + sys.name;
					if (!pass(path))
						continue;

					anyAddable = true;
					addSystem(sys, path.c_str());
				}
			}
			else
			{
				const char *categories[] = {"Camera", "Rendering", "Animation", "Physics", "UI", "Gameplay"};
				for (const char *category : categories)
				{
					if (!ImGui::BeginMenu(category))
						continue;

					const char *subfolders[4] = {};
					int subfolderCount = 0;
					if (std::string(category) == "Camera")
					{
						subfolders[0] = "Control";
						subfolderCount = 1;
					}
					else if (std::string(category) == "Rendering")
					{
						subfolders[0] = "2D";
						subfolders[1] = "3D";
						subfolderCount = 2;
					}
					else if (std::string(category) == "Animation")
					{
						subfolders[0] = "2D";
						subfolderCount = 1;
					}
					else if (std::string(category) == "Physics")
					{
						subfolders[0] = "2D";
						subfolderCount = 1;
					}
					else if (std::string(category) == "UI")
					{
						subfolders[0] = "Core";
						subfolderCount = 1;
					}
					else
					{
						subfolders[0] = "Core";
						subfolderCount = 1;
					}

					bool hasAnyInCategory = false;
					for (auto &sys : systems)
					{
						const bool isRemoved = removedSystems.find(sys.name) != removedSystems.end();
						const bool isEnabled = (sys.enabled && *sys.enabled);
						if (!isRemoved && isEnabled)
							continue;

						const std::string folderPath = folderPathForSystem(sys.name);
						if (folderPath.rfind(std::string(category) + "/", 0) == 0)
						{
							hasAnyInCategory = true;
							break;
						}
					}

					for (int subfolderIndex = 0; subfolderIndex < subfolderCount; ++subfolderIndex)
					{
						const char *subfolder = subfolders[subfolderIndex];
						if (!ImGui::BeginMenu(subfolder))
							continue;

						bool hasAnyInSubfolder = false;
						const std::string expectedPrefix = std::string(category) + "/" + subfolder;
						for (auto &sys : systems)
						{
							const bool isRemoved = removedSystems.find(sys.name) != removedSystems.end();
							const bool isEnabled = (sys.enabled && *sys.enabled);
							if (!isRemoved && isEnabled)
								continue;

							if (std::string(folderPathForSystem(sys.name)) != expectedPrefix)
								continue;

							hasAnyInSubfolder = true;
							addSystem(sys, sys.name);
						}

						if (!hasAnyInSubfolder)
							ImGui::MenuItem("No systems", nullptr, false, false);
						ImGui::EndMenu();
					}

					if (!hasAnyInCategory)
						ImGui::MenuItem("No systems", nullptr, false, false);
					anyAddable |= hasAnyInCategory;
					ImGui::EndMenu();
				}
			}

			if (!anyAddable)
				ImGui::TextUnformatted("No systems available to add.");

			ImGui::EndPopup();
		}
	}

	void DrawTopBarImpl(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene)
	{
		if (!std::isfinite(ui.dragSnapStep) || ui.dragSnapStep <= 0.0f)
			ui.dragSnapStep = 0.5f;

		if (!ImGui::BeginMainMenuBar())
			return;

		DrawSceneFilePopupsImpl(scenes, ui);

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save Selected Scene..."))
				ui.saveScenePopupOpen = true;
			if (ImGui::MenuItem("Open Scene From Disk..."))
				ui.openScenePopupOpen = true;
			ImGui::EndMenu();
		}

		const bool canEdit = (editorScene != nullptr);

		if (ImGui::BeginMenu("Edit"))
		{
			TransformUndoRedoService::DrawUndoRedoMenuItems(editorScene);

			ImGui::Separator();

			if (ImGui::MenuItem("Duplicate", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
			{
				Registry &registry = editorScene->GetEditorRegistry();
				se.selectedEntity = SceneEditor::DuplicateEntityRecursive(registry, se.selectedEntity);
			}

			if (ImGui::MenuItem("Delete", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
			{
				Registry &registry = editorScene->GetEditorRegistry();
				SceneEditor::DestroyEntityRecursive(registry, se.selectedEntity);
				se.selectedEntity = kInvalidEntity;
			}

			if (ImGui::MenuItem("Rename", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
				SceneEditor::BeginRename(editorScene->GetEditorRegistry(), se, se.selectedEntity);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("SceneList", nullptr, &ui.showSceneList);
			ImGui::MenuItem("Entity Hierarchy", nullptr, &ui.showEntityHierarchy);
			ImGui::MenuItem("Systems", nullptr, &ui.showSystems);
			ImGui::MenuItem("Console", nullptr, &ui.showConsole);
			ImGui::MenuItem("Inspector", nullptr, &ui.showInspector);
			ImGui::MenuItem("Render", nullptr, &ui.showRender);
#ifdef IMGUI_HAS_DOCK
			if (ImGui::MenuItem("Reset Layout"))
				ui.dockLayoutBuilt = false;
#endif
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Config"))
		{
			ImGui::MenuItem("Debug Clicks", nullptr, &ShowGizmoDebug());
			ImGui::MenuItem("Forward Arrow", nullptr, &ShowForwardArrow());
			const char *consoleLevels[] = {"Debug", "Info", "Warning", "Error"};
			int consoleLevelIndex = static_cast<int>(Console::GetLevel());
			ImGui::SetNextItemWidth(160.0f);
			if (ImGui::Combo("Console Level", &consoleLevelIndex, consoleLevels, IM_ARRAYSIZE(consoleLevels)))
				Console::SetLevel(static_cast<ConsoleLevel>(consoleLevelIndex));
			if (ImGui::BeginMenu("Physics2D"))
			{
				if (editorScene)
				{
					glm::vec2 gravity = editorScene->GetPhysics2DGravity();
					ImGui::SetNextItemWidth(170.0f);
					if (ImGui::DragFloat2("Gravity2D", &gravity.x, 0.05f, -1000.0f, 1000.0f, "%.3f"))
						editorScene->SetPhysics2DGravity(gravity);

					ImGui::MenuItem("Show 2DCollisions Gizmos", nullptr, &ShowCollisionGizmos());
				}
				else
				{
					ImGui::MenuItem("Gravity2D", nullptr, false, false);
					ImGui::MenuItem("Show 2DCollisions Gizmos", nullptr, false, false);
				}

				ImGui::EndMenu();
			}
			ImGui::Separator();
			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::DragFloat("Ctrl Drag Snap", &ui.dragSnapStep, 0.001f, 0.0001f, 1000.0f, "%.4f"))
				ui.dragSnapStep = std::max(0.0001f, ui.dragSnapStep);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add"))
		{
			if (ImGui::MenuItem("Entity", nullptr, false, canEdit))
			{
				Registry &registry = editorScene->GetEditorRegistry();
				se.selectedEntity = SceneEditor::CreateEntity(registry, "New Entity", kInvalidEntity, true);
			}

			if (ImGui::MenuItem("Child Entity", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
			{
				Registry &registry = editorScene->GetEditorRegistry();
				se.selectedEntity = SceneEditor::CreateEntity(registry, "New Entity", se.selectedEntity, true);
			}

			if (ImGui::BeginMenu("Prefab"))
			{
				if (!canEdit)
				{
					ImGui::MenuItem("Requires editable scene", nullptr, false, false);
				}
				else
				{
					std::vector<std::string> prefabNames;
					editorScene->GetPrefabRegistry().GetNames(prefabNames);
					if (prefabNames.empty())
					{
						ImGui::MenuItem("No prefabs registered", nullptr, false, false);
					}
					else
					{
						for (const std::string &prefabName : prefabNames)
						{
							ImGui::PushID(prefabName.c_str());
							if (ImGui::MenuItem(prefabName.c_str()))
								se.selectedEntity = AddPrefab(editorScene, se, prefabName);
							ImGui::PopID();
						}
					}

					if (editorScene->GetPrefabRegistry().Has("EnemyBasic"))
					{
						ImGui::Separator();
						if (ImGui::MenuItem("EnemyBasic (Override Tint/Size)"))
						{
							prefab::PrefabSpawnOverrides overrides;
							overrides.spriteTint = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
							overrides.spriteSize = glm::vec2(1.4f, 1.4f);
							se.selectedEntity = AddPrefab(editorScene, se, "EnemyBasic", &overrides);
						}
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Add Basic Shape"))
			{
				const bool canAddShape = canEdit && IsEditable3DScene(editorScene);
				if (!canAddShape)
				{
					ImGui::MenuItem("Requires an editable 3D scene", nullptr, false, false);
				}
				else
				{
					const std::vector<BasicShapeEntry> shapes = CollectBasicShapeEntries();
					if (shapes.empty())
					{
						ImGui::MenuItem("No models found in res/models/basicShapes", nullptr, false, false);
					}
					else
					{
						for (const BasicShapeEntry &shape : shapes)
						{
							ImGui::PushID(shape.meshPath.c_str());
							if (ImGui::MenuItem(shape.label.c_str()))
								se.selectedEntity = AddBasicShape(editorScene, se, shape.label, shape.meshPath);
							ImGui::PopID();
						}
					}
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Add Scene"))
			{
				ui.selectedScene = scenes.GetLoadedSceneCount();
				scenes.AddScene(SceneRequest::BasicScene);
			}

			if (ImGui::MenuItem("Add HUD Demo Scene"))
			{
				ui.selectedScene = scenes.GetLoadedSceneCount();
				scenes.AddScene(SceneRequest::HudDemoScene);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			bool profilerWindowOpen = EditorUI::IsProfilerWindowOpen();
			if (ImGui::MenuItem("Profiler", nullptr, profilerWindowOpen))
				EditorUI::SetProfilerWindowOpen(!profilerWindowOpen);

			if (ImGui::MenuItem("SpriteSheet Generator"))
				SpriteSheetGenerator().open = true;
			ImGui::EndMenu();
		}

		ImGui::Separator();
		ImGui::Text("Scene: %zu", ui.selectedScene);

		if (scenes.IsTransitioning())
		{
			ImGui::SameLine();
			ImGui::TextUnformatted("Transitioning...");
		}

		if (!ui.sceneFileStatus.empty())
		{
			ImGui::SameLine();
			ImGui::Text("| %s", ui.sceneFileStatus.c_str());
		}

		ImGui::EndMainMenuBar();
	}

	void DrawSystemsPanelImpl(IEditorScene *editorScene)
	{
		if (!editorScene)
		{
			ImGui::TextUnformatted("Scene is not editor-inspectable.");
			return;
		}

		if (ImGui::Button("Add System"))
			ImGui::OpenPopup("AddSystemPopup");

		AddSystemPopup(editorScene);

		ImGui::Separator();

		std::vector<EditorSystemToggle> systems;
		editorScene->GetEditorSystems(systems);
		auto &removedSystems = RemovedSystemsByScene()[editorScene];

		for (auto &system : systems)
		{
			if (removedSystems.find(system.name) != removedSystems.end())
				continue;

			ImGui::PushID(system.name);
			if (system.enabled)
				ImGui::Checkbox("##enabled", system.enabled);
			else
			{
				bool disabled = false;
				ImGui::BeginDisabled();
				ImGui::Checkbox("##enabled", &disabled);
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			ImGui::TextUnformatted(system.name);
			ImGui::SameLine();
			if (ImGui::SmallButton("Remove"))
			{
				removedSystems.insert(system.name);
				if (system.enabled)
					*system.enabled = false;
			}
			ImGui::PopID();
		}
	}
}

namespace editorui
{
	void TopBar::Draw(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene)
	{
		internal::DrawTopBarImpl(scenes, ui, se, editorScene);
	}

	void SystemsPanel::Draw(IEditorScene *editorScene)
	{
		internal::DrawSystemsPanelImpl(editorScene);
	}
}
