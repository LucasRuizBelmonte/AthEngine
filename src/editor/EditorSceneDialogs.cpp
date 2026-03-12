#include "EditorModulesInternal.h"

#include "../scene/SceneManager.h"

#include <imgui.h>

#include <cstdio>
#include <string>
#include <vector>

namespace editorui::internal
{
	void DrawSceneFilePopupsImpl(SceneManager &scenes, EditorUIState &ui)
	{
		static std::vector<std::string> saveSceneEntries;
		static std::vector<std::string> saveSceneEntriesLower;
		static char saveSceneFilter[128] = {};
		static int saveSceneSelection = -1;
		static bool refreshSaveSceneList = false;
		static std::vector<std::string> openSceneEntries;
		static std::vector<std::string> openSceneEntriesLower;
		static char openSceneFilter[128] = {};
		static int openSceneSelection = -1;
		static bool refreshOpenSceneList = false;

		auto trySaveSelectedScene = [&]()
		{
			std::string err;
			if (scenes.SaveLoadedSceneToFile(ui.selectedScene, ui.savePathBuf, err))
				ui.sceneFileStatus = "Scene saved.";
			else
				ui.sceneFileStatus = "Save failed: " + err;
		};

		if (ui.saveScenePopupOpen)
		{
			ImGui::OpenPopup("Save Scene");
			ui.saveScenePopupOpen = false;
			refreshSaveSceneList = true;
		}
		if (ui.openScenePopupOpen)
		{
			ImGui::OpenPopup("Open Scene");
			ui.openScenePopupOpen = false;
			refreshOpenSceneList = true;
		}

		if (ImGui::BeginPopupModal("Save Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (refreshSaveSceneList)
			{
				const auto &cached = CollectSceneAssetEntries(true);
				saveSceneEntries.assign(cached.begin(), cached.end());
				saveSceneEntriesLower.clear();
				saveSceneEntriesLower.reserve(saveSceneEntries.size());
				for (const std::string &entry : saveSceneEntries)
					saveSceneEntriesLower.push_back(ToLowerCopy(entry));
				saveSceneFilter[0] = '\0';
				saveSceneSelection = -1;
				const std::string current = ui.savePathBuf;
				for (size_t i = 0; i < saveSceneEntries.size(); ++i)
				{
					if (saveSceneEntries[i] == current)
					{
						saveSceneSelection = static_cast<int>(i);
						break;
					}
				}
				refreshSaveSceneList = false;
			}

			ImGui::InputText("Path", ui.savePathBuf, sizeof(ui.savePathBuf));
			ImGui::SameLine();
			if (ImGui::Button("Refresh"))
				refreshSaveSceneList = true;

			ImGui::InputTextWithHint("Search", "Filter .athscene files...", saveSceneFilter, sizeof(saveSceneFilter));

			const std::string filter = ToLowerCopy(saveSceneFilter);
			ImGui::BeginChild("SaveSceneList", ImVec2(680.0f, 260.0f), true);
			for (int i = 0; i < static_cast<int>(saveSceneEntries.size()); ++i)
			{
				const std::string &path = saveSceneEntries[static_cast<size_t>(i)];
				if (!filter.empty())
				{
					if (saveSceneEntriesLower[static_cast<size_t>(i)].find(filter) == std::string::npos)
						continue;
				}

				const bool selected = (saveSceneSelection == i);
				if (ImGui::Selectable(path.c_str(), selected))
				{
					saveSceneSelection = i;
					std::snprintf(ui.savePathBuf, sizeof(ui.savePathBuf), "%s", path.c_str());
				}
				if (selected && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					trySaveSelectedScene();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndChild();

			if (ImGui::Button("Save"))
			{
				trySaveSelectedScene();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (refreshOpenSceneList)
			{
				const auto &cached = CollectSceneAssetEntries(true);
				openSceneEntries.assign(cached.begin(), cached.end());
				openSceneEntriesLower.clear();
				openSceneEntriesLower.reserve(openSceneEntries.size());
				for (const std::string &entry : openSceneEntries)
					openSceneEntriesLower.push_back(ToLowerCopy(entry));
				openSceneFilter[0] = '\0';
				openSceneSelection = -1;
				const std::string current = ui.openPathBuf;
				for (size_t i = 0; i < openSceneEntries.size(); ++i)
				{
					if (openSceneEntries[i] == current)
					{
						openSceneSelection = static_cast<int>(i);
						break;
					}
				}
				refreshOpenSceneList = false;
			}

			ImGui::InputText("Path", ui.openPathBuf, sizeof(ui.openPathBuf));
			ImGui::SameLine();
			if (ImGui::Button("Refresh"))
				refreshOpenSceneList = true;

			ImGui::InputTextWithHint("Search", "Filter .athscene files...", openSceneFilter, sizeof(openSceneFilter));

			const std::string filter = ToLowerCopy(openSceneFilter);
			ImGui::BeginChild("OpenSceneList", ImVec2(680.0f, 260.0f), true);
			for (int i = 0; i < static_cast<int>(openSceneEntries.size()); ++i)
			{
				const std::string &path = openSceneEntries[static_cast<size_t>(i)];
				if (!filter.empty())
				{
					if (openSceneEntriesLower[static_cast<size_t>(i)].find(filter) == std::string::npos)
						continue;
				}

				const bool selected = (openSceneSelection == i);
				if (ImGui::Selectable(path.c_str(), selected))
				{
					openSceneSelection = i;
					std::snprintf(ui.openPathBuf, sizeof(ui.openPathBuf), "%s", path.c_str());
				}
				if (selected && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					std::string err;
					if (scenes.QueueOpenSceneFromFile(ui.openPathBuf, err))
					{
						ui.selectedScene = scenes.GetLoadedSceneCount();
						ui.sceneFileStatus = "Loading scene...";
					}
					else
					{
						ui.sceneFileStatus = "Open failed: " + err;
					}
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndChild();

			if (ImGui::Button("Open"))
			{
				std::string err;
				if (scenes.QueueOpenSceneFromFile(ui.openPathBuf, err))
				{
					ui.selectedScene = scenes.GetLoadedSceneCount();
					ui.sceneFileStatus = "Loading scene...";
				}
				else
				{
					ui.sceneFileStatus = "Open failed: " + err;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}
}

namespace editorui
{
	void SceneFileDialogs::Draw(SceneManager &scenes, EditorUIState &ui)
	{
		internal::DrawSceneFilePopupsImpl(scenes, ui);
	}
}
