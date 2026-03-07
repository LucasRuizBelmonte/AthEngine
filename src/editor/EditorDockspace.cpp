#include "EditorModulesInternal.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace editorui::internal
{
#ifdef IMGUI_HAS_DOCK
	static void BuildDefaultDock(ImGuiID dockspaceId)
	{
		ImGuiViewport *vp = ImGui::GetMainViewport();

		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, vp->Size);

		ImGuiID dockMain = dockspaceId;
		ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.25f, nullptr, &dockMain);
		ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.30f, nullptr, &dockMain);
		ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.25f, nullptr, &dockMain);
		ImGuiID dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.40f, nullptr, &dockLeft);
		ImGuiID dockBottomRight = ImGui::DockBuilderSplitNode(dockBottom, ImGuiDir_Right, 0.50f, nullptr, &dockBottom);

		ImGui::DockBuilderDockWindow("SceneList", dockLeft);
		ImGui::DockBuilderDockWindow("Entity Hierarchy", dockLeftBottom);
		ImGui::DockBuilderDockWindow("Systems", dockBottom);
		ImGui::DockBuilderDockWindow("Console", dockBottomRight);
		ImGui::DockBuilderDockWindow("Inspector", dockRight);
		ImGui::DockBuilderDockWindow("Render", dockMain);

		ImGui::DockBuilderFinish(dockspaceId);
	}
#endif

	void DrawDockSpaceImpl(EditorUIState &state)
	{
#ifdef IMGUI_HAS_DOCK
		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		ImGuiViewport *vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->WorkPos);
		ImGui::SetNextWindowSize(vp->WorkSize);
		ImGui::SetNextWindowViewport(vp->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::Begin("DockSpaceHost", nullptr, flags);

		ImGui::PopStyleVar(3);

		ImGuiID dockspaceId = ImGui::GetID("AthEngineDockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

		if (!state.dockLayoutBuilt)
		{
			state.dockLayoutBuilt = true;
			BuildDefaultDock(dockspaceId);
		}

		ImGui::End();
#endif
	}
}

namespace editorui
{
	void Dockspace::Draw(EditorUIState &state)
	{
		internal::DrawDockSpaceImpl(state);
	}
}
