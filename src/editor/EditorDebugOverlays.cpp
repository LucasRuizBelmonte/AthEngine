#include "EditorModulesInternal.h"

#include "../components/Tag.h"
#include "../input/Input.h"
#include "../scene/IEditorScene.h"
#include "../utils/Console.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdio>
#include <string>
#include <vector>

namespace editorui::internal
{
	namespace
	{
		bool HasInputDebugPanelMarker(IEditorScene *editorScene)
		{
			if (!editorScene)
				return false;

			Registry &registry = editorScene->GetEditorRegistry();
			std::vector<Entity> taggedEntities;
			registry.ViewEntities<Tag>(taggedEntities);
			for (Entity entity : taggedEntities)
			{
				const Tag &tag = registry.Get<Tag>(entity);
				if (tag.name == "InputDebugPanel")
					return true;
			}

			return false;
		}

		bool IsConsoleEntryVisible(ConsoleLevel entryLevel, ConsoleLevel filterLevel)
		{
			return static_cast<int>(entryLevel) >= static_cast<int>(filterLevel);
		}

		const char *ConsoleLevelLabel(ConsoleLevel level)
		{
			switch (level)
			{
			case ConsoleLevel::Debug:
				return "Debug";
			case ConsoleLevel::Info:
				return "Info";
			case ConsoleLevel::Warning:
				return "Warning";
			case ConsoleLevel::Error:
				return "Error";
			}
			return "Debug";
		}

		ImVec4 ConsoleLevelColor(ConsoleLevel level)
		{
			switch (level)
			{
			case ConsoleLevel::Warning:
				return ImVec4(1.0f, 0.95f, 0.35f, 1.0f);
			case ConsoleLevel::Error:
				return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
			case ConsoleLevel::Info:
				return ImVec4(0.45f, 0.65f, 1.0f, 1.0f);
			case ConsoleLevel::Debug:
			default:
				return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
	}

	void DrawInputActionDebugPanelImpl(IEditorScene *editorScene)
	{
		if (!HasInputDebugPanelMarker(editorScene))
			return;

		const InputState &input = Input::GetState();
		const std::vector<std::string> &actions = input.GetActionNames();

		ImGui::Begin("Input Actions");
		ImGui::TextUnformatted("Action map runtime state");
		ImGui::Separator();

		if (ImGui::BeginTable("##InputActionState", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Action");
			ImGui::TableSetupColumn("Axis");
			ImGui::TableSetupColumn("Down");
			ImGui::TableSetupColumn("Pressed");
			ImGui::TableSetupColumn("Released");
			ImGui::TableHeadersRow();

			for (const std::string &actionName : actions)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(actionName.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.3f", input.GetAxis(actionName));
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(input.GetDown(actionName) ? "true" : "false");
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(input.GetPressed(actionName) ? "true" : "false");
				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(input.GetReleased(actionName) ? "true" : "false");
			}

			ImGui::EndTable();
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Preset editable in: src/input/ProjectInputMap.cpp");
		ImGui::End();
	}

	void DrawConsolePanelImpl()
	{
		if (ImGui::Button("Clear"))
			Console::Clear();

		ImGui::Separator();

		const ConsoleLevel filterLevel = Console::GetLevel();
		std::vector<ConsoleEntry> entries = Console::GetEntries();
		std::vector<const ConsoleEntry *> visibleEntries;
		visibleEntries.reserve(entries.size());
		for (const ConsoleEntry &entry : entries)
		{
			if (IsConsoleEntryVisible(entry.level, filterLevel))
				visibleEntries.push_back(&entry);
		}

		ImGui::BeginChild("ConsoleEntries", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
		const bool autoScroll = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(visibleEntries.size()));
		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
			{
				const ConsoleEntry &entry = *visibleEntries[static_cast<size_t>(i)];
				ImGui::TextColored(ConsoleLevelColor(entry.level),
				                   "[%s] %s",
				                   ConsoleLevelLabel(entry.level),
				                   entry.message.c_str());
			}
		}

		if (autoScroll)
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
	}

	void DrawGizmoDebugOverlayImpl(bool viewportFocused, bool imageHovered, const ImVec2 &imageMin)
	{
		if (!ShowGizmoDebug())
			return;

		ImGuiIO &io = ImGui::GetIO();
		ImGuiContext &imguiCtx = *ImGui::GetCurrentContext();
		ImGuiWindow *hoveredWindow = imguiCtx.HoveredWindow;
		const char *hoveredName = hoveredWindow ? hoveredWindow->Name : "<none>";
		const ImGuiID hoveredId = ImGui::GetHoveredID();
		const ImGuiID activeId = ImGui::GetActiveID();

		ImGui::SetCursorScreenPos(ImVec2(imageMin.x + 8.0f, imageMin.y + 8.0f));
		ImGui::BeginChild("##GizmoDebugOverlay",
		                  ImVec2(430.0f, 148.0f),
		                  true,
		                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);
		ImGui::Text("Focused:%d  ImageHovered:%d  WantCaptureMouse:%d",
		            viewportFocused ? 1 : 0,
		            imageHovered ? 1 : 0,
		            io.WantCaptureMouse ? 1 : 0);
		ImGui::Text("Target:%d  GizmoEnabled:%d  GizmoOver:%d  GizmoUsing:%d",
		            GizmoDebug().hasTarget ? 1 : 0,
		            GizmoDebug().enabled ? 1 : 0,
		            GizmoDebug().isOver ? 1 : 0,
		            GizmoDebug().isUsing ? 1 : 0);
		ImGui::Text("Mouse:(%.1f, %.1f)", io.MousePos.x, io.MousePos.y);
		ImGui::Text("HoveredWindow:%s", hoveredName);
		ImGui::Text("HoveredID:0x%08X  ActiveID:0x%08X", hoveredId, activeId);
		ImGui::EndChild();

		if (imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			std::printf("[GizmoClick] Focused=%d ImageHovered=%d WantCaptureMouse=%d Target=%d Enabled=%d Over=%d Using=%d Mouse=(%.1f,%.1f) HoveredWindow=%s HoveredID=0x%08X ActiveID=0x%08X\n",
			            viewportFocused ? 1 : 0,
			            imageHovered ? 1 : 0,
			            io.WantCaptureMouse ? 1 : 0,
			            GizmoDebug().hasTarget ? 1 : 0,
			            GizmoDebug().enabled ? 1 : 0,
			            GizmoDebug().isOver ? 1 : 0,
			            GizmoDebug().isUsing ? 1 : 0,
			            io.MousePos.x,
			            io.MousePos.y,
			            hoveredName,
			            hoveredId,
			            activeId);
		}
	}
}

namespace editorui
{
	void GizmoDebugOverlay::Draw(bool viewportFocused, bool imageHovered, const ImVec2 &imageMin)
	{
		internal::DrawGizmoDebugOverlayImpl(viewportFocused, imageHovered, imageMin);
	}
}
