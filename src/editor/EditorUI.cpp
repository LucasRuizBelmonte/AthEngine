#pragma region Includes
#include "EditorUI.h"

#include "EditorModules.h"
#pragma endregion

#pragma region File Scope
namespace
{
	bool g_profilerWindowOpen = false;
}
#pragma endregion

#pragma region Function Definitions
void EditorUI::SetRenderTexture(ImTextureID textureId)
{
	editorui::Editor::SetRenderTexture(textureId);
}

ImVec2 EditorUI::GetRenderTargetSize()
{
	return editorui::Editor::GetRenderTargetSize();
}

bool EditorUI::IsRenderWindowHovered()
{
	return editorui::Editor::IsRenderWindowHovered();
}

void EditorUI::Draw(SceneManager &scenes, EditorUIState &state)
{
	editorui::Editor::Draw(scenes, state);
}

void EditorUI::SetProfilerWindowOpen(bool open)
{
	g_profilerWindowOpen = open;
}

bool EditorUI::IsProfilerWindowOpen()
{
	return g_profilerWindowOpen;
}
#pragma endregion
