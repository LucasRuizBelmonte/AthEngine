#include "EditorModulesInternal.h"

#include <imgui.h>

#include <algorithm>

namespace editorui::internal
{
	void DrawViewportPanelImpl(EditorUIState &state, IEditorScene *editorScene, SceneEditorState &se)
	{
		ImGuiWindowFlags renderFlags =
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Render", &state.showRender, renderFlags);
		RenderWindowHovered() = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

		const bool viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::BeginChild("##RenderToolbar", ImVec2(0.0f, 64.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		DrawGizmoToolbarImpl(viewportFocused);
		ImGui::EndChild();
		ImGui::PopStyleVar();

		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.x < 1.0f)
			avail.x = 1.0f;
		if (avail.y < 1.0f)
			avail.y = 1.0f;

		RenderTargetSize() = avail;

		if (RenderTexture())
		{
			ImGui::Image(RenderTexture(), avail, ImVec2(0, 1), ImVec2(1, 0));
			const ImVec2 imageMin = ImGui::GetItemRectMin();
			const ImVec2 imageSize = ImGui::GetItemRectSize();
			const bool imageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			DrawDebugVisualizationLayerImpl(editorScene, se, imageMin, imageSize);
			DrawTransformGizmoImpl(editorScene, se, imageMin, imageSize, viewportFocused, imageHovered);
			DrawGizmoDebugOverlayImpl(viewportFocused, imageHovered, imageMin);
		}
		else
		{
			ImGui::TextUnformatted("Render target not ready.");
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}

namespace editorui
{
	RenderTargetView::View RenderTargetView::Draw(ImTextureID textureId, ImVec2 &outRenderTargetSize)
	{
		RenderTargetView::View view;

		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.x < 1.0f)
			avail.x = 1.0f;
		if (avail.y < 1.0f)
			avail.y = 1.0f;

		outRenderTargetSize = avail;

		if (textureId)
		{
			ImGui::Image(textureId, avail, ImVec2(0, 1), ImVec2(1, 0));
			view.imageMin = ImGui::GetItemRectMin();
			view.imageSize = ImGui::GetItemRectSize();
			view.imageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		}
		return view;
	}

	void ViewportPanel::SetRenderTexture(ImTextureID textureId)
	{
		internal::RenderTexture() = textureId;
	}

	ImVec2 ViewportPanel::GetRenderTargetSize()
	{
		return internal::RenderTargetSize();
	}

	bool ViewportPanel::IsRenderWindowHovered()
	{
		return internal::RenderWindowHovered();
	}

	void ViewportPanel::Draw(EditorUIState &state, IEditorScene *editorScene, SceneEditorState &se)
	{
		internal::DrawViewportPanelImpl(state, editorScene, se);
	}
}
