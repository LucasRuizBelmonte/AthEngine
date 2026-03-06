#pragma region Includes
#include "EditorModules.h"

#include <imgui.h>
#include <imgui_internal.h>
#include "../platform/GL.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../scene/SceneManager.h"
#include "../scene/IScene.h"
#include "../scene/IEditorScene.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Transform.h"
#include "../components/Parent.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Tag.h"
#include "../input/Input.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/Physics2DAlgorithms.h"
#include "../physics2d/PhysicsBody2D.h"
#include "../physics2d/RigidBody2D.h"
#include "../prefab/PrefabRegistry.h"
#include "../thirdparty/ImGuizmo.h"
#include "../thirdparty/stb_image.h"
#include "SceneEditor.h"
#pragma endregion

namespace editorui
{
#pragma region File Scope
	static ImTextureID g_renderTexture = nullptr;
	static ImVec2 g_renderTargetSize = ImVec2(1.0f, 1.0f);
	static bool g_renderWindowHovered = false;
	static std::unordered_map<IEditorScene *, std::unordered_set<std::string>> g_removedSystemsByScene;

	struct TransformEditCommand
	{
		Entity entity = kInvalidEntity;
		Transform before{};
		Transform after{};
	};

	struct TransformHistory
	{
		std::vector<TransformEditCommand> undoStack;
		std::vector<TransformEditCommand> redoStack;
	};

	struct GizmoState
	{
		ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
		ImGuizmo::MODE mode = ImGuizmo::LOCAL;
		bool isManipulating = false;
		IEditorScene *activeScene = nullptr;
		Entity activeEntity = kInvalidEntity;
		Transform beginTransform{};
		glm::mat4 activeWorldMatrix{1.f};
		bool hasActiveWorldMatrix = false;
	};

	struct GizmoRuntimeDebug
	{
		bool hasTarget = false;
		bool enabled = false;
		bool isOver = false;
		bool isUsing = false;
	};

	static std::unordered_map<IEditorScene *, TransformHistory> g_transformHistoryByScene;
	static GizmoState g_gizmoState;
	static GizmoRuntimeDebug g_gizmoRuntimeDebug;
	static bool g_showGizmoDebug = false;
	static bool g_showForwardArrow = true;
	static bool g_showCollisionGizmos = true;

	struct GizmoRigidBodyOverride
	{
		IEditorScene *scene = nullptr;
		Entity entity = kInvalidEntity;
		bool hasOverride = false;
		bool originalIsKinematic = false;
	};

	static GizmoRigidBodyOverride g_gizmoRigidBodyOverride;

	struct SpriteSheetSourceImage
	{
		std::string inputPath;
		std::string resolvedPath;
		int width = 0;
		int height = 0;
		std::vector<uint8_t> rgba;
	};

	struct SpriteSheetComposite
	{
		int width = 0;
		int height = 0;
		int cellWidth = 0;
		int cellHeight = 0;
		int usedRows = 0;
		int placedCount = 0;
		std::vector<uint8_t> rgba;
	};

	struct SpriteSheetGeneratorState
	{
		bool open = false;
		std::vector<std::string> textureAssetEntries;
		int selectedTextureAsset = -1;
		char spritePathBuf[512] = {};
		char outputPathBuf[512] = "res/textures/generated_sprite_sheet.tga";
		int columns = 4;
		int rows = 4;
		bool autoExpandRows = true;
		int gapX = 0;
		int gapY = 0;
		int marginX = 0;
		int marginY = 0;
		std::vector<SpriteSheetSourceImage> sources;
		SpriteSheetComposite preview;
		GLuint previewTexture = 0;
		uint64_t dirtyRevision = 1;
		uint64_t builtRevision = 0;
		std::string status;
	};

	static SpriteSheetGeneratorState g_spriteSheetGenerator;

	static Entity FindEditorCameraEntity(Registry &r);
#pragma endregion

#pragma region Function Definitions
	void Editor::SetRenderTexture(ImTextureID textureId)
	{
		g_renderTexture = textureId;
	}

	ImVec2 Editor::GetRenderTargetSize()
	{
		return g_renderTargetSize;
	}

	bool Editor::IsRenderWindowHovered()
	{
		return g_renderWindowHovered;
	}

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

		ImGui::DockBuilderDockWindow("SceneList", dockLeft);
		ImGui::DockBuilderDockWindow("Entity Hierarchy", dockLeftBottom);
		ImGui::DockBuilderDockWindow("Systems", dockBottom);
		ImGui::DockBuilderDockWindow("Inspector", dockRight);
		ImGui::DockBuilderDockWindow("Render", dockMain);

		ImGui::DockBuilderFinish(dockspaceId);
	}
#endif

	static void DrawDockSpace(EditorUIState &state)
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

	static std::string TrimCopy(std::string text)
	{
		const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char c)
											{ return std::isspace(c) != 0; });
		const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c)
										  { return std::isspace(c) != 0; })
							 .base();
		if (begin >= end)
			return {};
		return std::string(begin, end);
	}

	static std::string ToLowerCopy(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
					   { return static_cast<char>(std::tolower(c)); });
		return text;
	}

	static std::vector<std::string> CollectSceneAssetEntries()
	{
		std::vector<std::string> entries;
		std::error_code ec;

		const std::filesystem::path assetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
		const std::filesystem::path projectRoot = assetRoot.parent_path();
		if (!std::filesystem::exists(assetRoot, ec))
			return entries;

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
			std::string ext = ToLowerCopy(candidate.extension().string());
			if (ext != ".athscene")
				continue;

			std::filesystem::path rel = std::filesystem::relative(candidate, projectRoot, ec);
			if (ec)
			{
				ec.clear();
				rel = candidate.lexically_normal();
			}

			entries.push_back(rel.generic_string());
		}

		std::sort(entries.begin(), entries.end());
		return entries;
	}

	struct BasicShapeEntry
	{
		std::string label;
		std::string meshPath;
	};

	static std::vector<BasicShapeEntry> CollectBasicShapeEntries()
	{
		std::vector<BasicShapeEntry> entries;
		std::error_code ec;

		const std::filesystem::path assetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
		const std::filesystem::path projectRoot = assetRoot.parent_path();
		const std::filesystem::path basicShapeRoot = assetRoot / "models" / "basicShapes";
		if (!std::filesystem::exists(basicShapeRoot, ec))
			return entries;

		std::filesystem::recursive_directory_iterator it(basicShapeRoot, std::filesystem::directory_options::skip_permission_denied, ec);
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
			const std::string ext = ToLowerCopy(candidate.extension().string());
			if (ext != ".fbx" && ext != ".obj" && ext != ".gltf" && ext != ".glb")
				continue;

			std::filesystem::path rel = std::filesystem::relative(candidate, projectRoot, ec);
			if (ec)
			{
				ec.clear();
				rel = candidate.lexically_normal();
			}

			entries.push_back(BasicShapeEntry{
				candidate.stem().string(),
				rel.generic_string()});
		}

		std::sort(entries.begin(), entries.end(), [](const BasicShapeEntry &a, const BasicShapeEntry &b)
				  { return ToLowerCopy(a.label) < ToLowerCopy(b.label); });
		return entries;
	}

	static std::vector<std::string> CollectTextureAssetEntries()
	{
		std::vector<std::string> entries;
		std::error_code ec;

		const std::filesystem::path assetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
		const std::filesystem::path projectRoot = assetRoot.parent_path();
		if (!std::filesystem::exists(assetRoot, ec))
			return entries;

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
			const std::string ext = ToLowerCopy(candidate.extension().string());
			if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp" && ext != ".tga")
				continue;

			std::filesystem::path rel = std::filesystem::relative(candidate, projectRoot, ec);
			if (ec)
			{
				ec.clear();
				rel = candidate.lexically_normal();
			}

			entries.push_back(rel.generic_string());
		}

		std::sort(entries.begin(), entries.end());
		return entries;
	}

	static std::string ResolveRuntimeAssetPath(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();
		if (path.is_absolute())
			return path.string();

		const std::string generic = path.generic_string();
		const std::filesystem::path assetRoot(ASSET_PATH);
		const std::filesystem::path projectRoot = assetRoot.parent_path();

		if (generic == "res" || generic.rfind("res/", 0) == 0)
			return (projectRoot / path).lexically_normal().string();

		return (assetRoot / path).lexically_normal().string();
	}

	static void DestroySpriteSheetPreviewTexture(SpriteSheetGeneratorState &state)
	{
		if (state.previewTexture != 0u)
		{
			GLuint id = state.previewTexture;
			glDeleteTextures(1, &id);
			state.previewTexture = 0u;
		}
	}

	static bool LoadSpriteSheetSourceImage(const std::string &inputPath, SpriteSheetSourceImage &outImage, std::string &outError)
	{
		const std::string resolvedPath = ResolveRuntimeAssetPath(inputPath);
		int width = 0;
		int height = 0;
		int channels = 0;
		stbi_set_flip_vertically_on_load(1);
		unsigned char *data = stbi_load(resolvedPath.c_str(), &width, &height, &channels, 4);
		if (!data || width <= 0 || height <= 0)
		{
			outError = "Could not load image: " + inputPath;
			if (data)
				stbi_image_free(data);
			return false;
		}

		outImage.inputPath = inputPath;
		outImage.resolvedPath = resolvedPath;
		outImage.width = width;
		outImage.height = height;
		outImage.rgba.assign(data, data + static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
		stbi_image_free(data);
		return true;
	}

	static bool IsSupportedImagePath(const std::string &path)
	{
		const std::string ext = ToLowerCopy(std::filesystem::path(path).extension().string());
		return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga";
	}

	static bool TryAddSourceToSpriteSheetGenerator(SpriteSheetGeneratorState &state,
	                                               const std::string &rawPath,
	                                               std::string &outMessage)
	{
		const std::string path = TrimCopy(rawPath);
		if (path.empty())
		{
			outMessage = "Ruta vacia.";
			return false;
		}

		if (!IsSupportedImagePath(path))
		{
			outMessage = "Formato no soportado: " + path;
			return false;
		}

		SpriteSheetSourceImage loaded;
		std::string loadError;
		if (!LoadSpriteSheetSourceImage(path, loaded, loadError))
		{
			outMessage = loadError;
			return false;
		}

		state.sources.push_back(std::move(loaded));
		++state.dirtyRevision;
		outMessage = "Sprite anadido: " + path;
		return true;
	}

	static bool TryReadPathFromDropPayload(const ImGuiPayload *payload, std::string &outPath)
	{
		outPath.clear();
		if (!payload || !payload->Data || payload->DataSize <= 0)
			return false;

		const char *data = static_cast<const char *>(payload->Data);
		int length = payload->DataSize;
		while (length > 0 && data[length - 1] == '\0')
			--length;
		if (length <= 0)
			return false;

		outPath.assign(data, data + length);
		return true;
	}

	static bool ComposeSpriteSheet(const SpriteSheetGeneratorState &state, SpriteSheetComposite &outComposite, std::string &outError)
	{
		outComposite = {};
		outError.clear();

		if (state.sources.empty())
		{
			outError = "No hay sprites en la lista.";
			return false;
		}

		const int columns = std::max(1, state.columns);
		int rows = std::max(1, state.rows);
		const int gapX = std::max(0, state.gapX);
		const int gapY = std::max(0, state.gapY);
		const int marginX = std::max(0, state.marginX);
		const int marginY = std::max(0, state.marginY);

		if (state.autoExpandRows)
			rows = std::max(rows, (static_cast<int>(state.sources.size()) + columns - 1) / columns);

		const int capacity = columns * rows;
		const int placedCount = std::min(static_cast<int>(state.sources.size()), capacity);
		if (placedCount <= 0)
		{
			outError = "No hay espacio para colocar sprites con columnas/filas actuales.";
			return false;
		}
		if (static_cast<int>(state.sources.size()) > capacity)
			outError = "Hay mas sprites que celdas. Se usaran solo los primeros.";

		int cellWidth = 1;
		int cellHeight = 1;
		for (const SpriteSheetSourceImage &source : state.sources)
		{
			cellWidth = std::max(cellWidth, source.width);
			cellHeight = std::max(cellHeight, source.height);
		}

		const int sheetWidth = marginX * 2 + columns * cellWidth + (columns - 1) * gapX;
		const int sheetHeight = marginY * 2 + rows * cellHeight + (rows - 1) * gapY;
		if (sheetWidth <= 0 || sheetHeight <= 0)
		{
			outError = "El tamano final del sprite sheet es invalido.";
			return false;
		}

		outComposite.width = sheetWidth;
		outComposite.height = sheetHeight;
		outComposite.cellWidth = cellWidth;
		outComposite.cellHeight = cellHeight;
		outComposite.usedRows = rows;
		outComposite.placedCount = placedCount;
		outComposite.rgba.assign(static_cast<size_t>(sheetWidth) * static_cast<size_t>(sheetHeight) * 4u, 0u);

		for (int i = 0; i < placedCount; ++i)
		{
			const SpriteSheetSourceImage &source = state.sources[static_cast<size_t>(i)];
			const int col = i % columns;
			const int row = i / columns;

			const int dstX = marginX + col * (cellWidth + gapX);
			const int dstY = marginY + row * (cellHeight + gapY);
			const int copyW = std::min(source.width, cellWidth);
			const int copyH = std::min(source.height, cellHeight);

			for (int y = 0; y < copyH; ++y)
			{
				for (int x = 0; x < copyW; ++x)
				{
					const int srcX = x;
					const int srcY = copyH - 1 - y;
					const size_t src = (static_cast<size_t>(srcY) * static_cast<size_t>(source.width) + static_cast<size_t>(srcX)) * 4u;
					const size_t dst = (static_cast<size_t>(dstY + y) * static_cast<size_t>(sheetWidth) + static_cast<size_t>(dstX + x)) * 4u;
					outComposite.rgba[dst + 0u] = source.rgba[src + 0u];
					outComposite.rgba[dst + 1u] = source.rgba[src + 1u];
					outComposite.rgba[dst + 2u] = source.rgba[src + 2u];
					outComposite.rgba[dst + 3u] = source.rgba[src + 3u];
				}
			}
		}

		return true;
	}

	static void UploadSpriteSheetPreviewTexture(SpriteSheetGeneratorState &state)
	{
		DestroySpriteSheetPreviewTexture(state);

		if (state.preview.width <= 0 || state.preview.height <= 0 || state.preview.rgba.empty())
			return;

		GLuint textureId = 0;
		glGenTextures(1, &textureId);
		if (textureId == 0u)
			return;

		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA8,
					 state.preview.width,
					 state.preview.height,
					 0,
					 GL_RGBA,
					 GL_UNSIGNED_BYTE,
					 state.preview.rgba.data());
		glBindTexture(GL_TEXTURE_2D, 0);

		state.previewTexture = textureId;
	}

	static bool WriteTgaRgba(const std::string &path,
							 int width,
							 int height,
							 const std::vector<uint8_t> &rgba,
							 std::string &outError)
	{
		outError.clear();
		if (width <= 0 || height <= 0)
		{
			outError = "Sprite sheet vacio.";
			return false;
		}

		const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
		if (rgba.size() < pixelCount * 4u)
		{
			outError = "Buffer RGBA incompleto.";
			return false;
		}

		std::error_code ec;
		std::filesystem::path outputPath(path);
		if (outputPath.has_parent_path())
			std::filesystem::create_directories(outputPath.parent_path(), ec);

		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out)
		{
			outError = "No se pudo crear el archivo: " + path;
			return false;
		}

		uint8_t header[18] = {};
		header[2] = 2;
		header[12] = static_cast<uint8_t>(width & 0xFF);
		header[13] = static_cast<uint8_t>((width >> 8) & 0xFF);
		header[14] = static_cast<uint8_t>(height & 0xFF);
		header[15] = static_cast<uint8_t>((height >> 8) & 0xFF);
		header[16] = 32;
		header[17] = 8 | 0x20;

		out.write(reinterpret_cast<const char *>(header), sizeof(header));

		std::vector<uint8_t> bgra(pixelCount * 4u);
		for (size_t i = 0; i < pixelCount; ++i)
		{
			const size_t src = i * 4u;
			const size_t dst = i * 4u;
			bgra[dst + 0u] = rgba[src + 2u];
			bgra[dst + 1u] = rgba[src + 1u];
			bgra[dst + 2u] = rgba[src + 0u];
			bgra[dst + 3u] = rgba[src + 3u];
		}

		out.write(reinterpret_cast<const char *>(bgra.data()), static_cast<std::streamsize>(bgra.size()));
		if (!out.good())
		{
			outError = "Error escribiendo el archivo: " + path;
			return false;
		}
		return true;
	}

	static void MarkSpriteSheetGeneratorDirty()
	{
		++g_spriteSheetGenerator.dirtyRevision;
	}

	static void RebuildSpriteSheetGeneratorPreview()
	{
		SpriteSheetComposite composite;
		std::string composeError;
		if (!ComposeSpriteSheet(g_spriteSheetGenerator, composite, composeError))
		{
			g_spriteSheetGenerator.preview = {};
			DestroySpriteSheetPreviewTexture(g_spriteSheetGenerator);
			g_spriteSheetGenerator.status = composeError;
			g_spriteSheetGenerator.builtRevision = g_spriteSheetGenerator.dirtyRevision;
			return;
		}

		g_spriteSheetGenerator.preview = std::move(composite);
		UploadSpriteSheetPreviewTexture(g_spriteSheetGenerator);
		g_spriteSheetGenerator.status = composeError;
		g_spriteSheetGenerator.builtRevision = g_spriteSheetGenerator.dirtyRevision;
	}

	static void DrawSpriteSheetGeneratorWindow()
	{
		SpriteSheetGeneratorState &state = g_spriteSheetGenerator;
		if (!state.open)
			return;

		if (state.textureAssetEntries.empty())
			state.textureAssetEntries = CollectTextureAssetEntries();

		if (!ImGui::Begin("SpriteSheet Generator", &state.open))
		{
			ImGui::End();
			return;
		}

		if (ImGui::Button("Refresh Texture Assets"))
		{
			state.textureAssetEntries = CollectTextureAssetEntries();
			if (state.selectedTextureAsset >= static_cast<int>(state.textureAssetEntries.size()))
				state.selectedTextureAsset = -1;
		}

		const char *assetPreview = (state.selectedTextureAsset >= 0 &&
									state.selectedTextureAsset < static_cast<int>(state.textureAssetEntries.size()))
									   ? state.textureAssetEntries[static_cast<size_t>(state.selectedTextureAsset)].c_str()
									   : "Select texture asset...";
		if (ImGui::BeginCombo("Texture Asset", assetPreview))
		{
			for (int i = 0; i < static_cast<int>(state.textureAssetEntries.size()); ++i)
			{
				const bool selected = (state.selectedTextureAsset == i);
				if (ImGui::Selectable(state.textureAssetEntries[static_cast<size_t>(i)].c_str(), selected))
					state.selectedTextureAsset = i;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Add Selected Asset") &&
			state.selectedTextureAsset >= 0 &&
			state.selectedTextureAsset < static_cast<int>(state.textureAssetEntries.size()))
		{
			const std::string selectedPath = state.textureAssetEntries[static_cast<size_t>(state.selectedTextureAsset)];
			std::string addMessage;
			(void)TryAddSourceToSpriteSheetGenerator(state, selectedPath, addMessage);
			state.status = addMessage;
		}

		ImGui::Separator();
		ImGui::InputText("Sprite Path", state.spritePathBuf, sizeof(state.spritePathBuf));
		ImGui::SameLine();
		if (ImGui::Button("Add Path"))
		{
			const std::string rawPath = TrimCopy(state.spritePathBuf);
			if (!rawPath.empty())
			{
				std::string addMessage;
				(void)TryAddSourceToSpriteSheetGenerator(state, rawPath, addMessage);
				state.status = addMessage;
			}
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Drag & Drop");
		ImGui::BeginChild("##SpriteDropZone", ImVec2(0.0f, 48.0f), true);
		ImGui::TextUnformatted("Suelta imagenes aqui (png/jpg/jpeg/bmp/tga).");
		ImGui::EndChild();

		bool dropHandled = false;
		int dropAddedCount = 0;
		int dropRejectedCount = 0;
		std::string lastDropError;

		auto processDropPath = [&](const std::string &droppedPath)
		{
			std::string addMessage;
			if (TryAddSourceToSpriteSheetGenerator(state, droppedPath, addMessage))
			{
				++dropAddedCount;
			}
			else
			{
				++dropRejectedCount;
				lastDropError = addMessage;
			}
		};

		if (ImGui::BeginDragDropTarget())
		{
			const char *payloadTypes[] = {
				"ATH_FILE_PATH",
				"ATH_ASSET_PATH",
				"ATH_PATH",
				"FILE_PATH"};

			for (const char *payloadType : payloadTypes)
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(payloadType))
				{
					std::string payloadPath;
					if (TryReadPathFromDropPayload(payload, payloadPath))
					{
						processDropPath(payloadPath);
						dropHandled = true;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		if ((ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) ||
		     ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) &&
		    Input::HasDroppedFiles())
		{
			std::vector<std::string> droppedFiles = Input::ConsumeDroppedFiles();
			for (const std::string &droppedPath : droppedFiles)
				processDropPath(droppedPath);
			dropHandled = !droppedFiles.empty();
		}

		if (dropHandled)
		{
			if (dropAddedCount > 0)
			{
				state.status = "Sprites anadidos por drag and drop: " + std::to_string(dropAddedCount);
				if (dropRejectedCount > 0)
					state.status += " (omitidos: " + std::to_string(dropRejectedCount) + ")";
			}
			else if (!lastDropError.empty())
			{
				state.status = lastDropError;
			}
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Sprites");
		ImGui::BeginChild("##SpriteList", ImVec2(0.0f, 180.0f), true);
		for (int i = 0; i < static_cast<int>(state.sources.size()); ++i)
		{
			ImGui::PushID(i);
			const SpriteSheetSourceImage &source = state.sources[static_cast<size_t>(i)];
			ImGui::Text("[%d] %s (%dx%d)", i, source.inputPath.c_str(), source.width, source.height);
			ImGui::SameLine();
			if (ImGui::SmallButton("Up") && i > 0)
			{
				std::swap(state.sources[static_cast<size_t>(i)], state.sources[static_cast<size_t>(i - 1)]);
				MarkSpriteSheetGeneratorDirty();
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Down") && i + 1 < static_cast<int>(state.sources.size()))
			{
				std::swap(state.sources[static_cast<size_t>(i)], state.sources[static_cast<size_t>(i + 1)]);
				MarkSpriteSheetGeneratorDirty();
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Remove"))
			{
				state.sources.erase(state.sources.begin() + i);
				MarkSpriteSheetGeneratorDirty();
				ImGui::PopID();
				--i;
				continue;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		if (ImGui::Button("Clear List"))
		{
			state.sources.clear();
			MarkSpriteSheetGeneratorDirty();
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Layout");
		bool changed = false;
		changed |= ImGui::InputInt("Columns", &state.columns);
		changed |= ImGui::InputInt("Rows", &state.rows);
		changed |= ImGui::Checkbox("Auto Expand Rows", &state.autoExpandRows);
		changed |= ImGui::InputInt("Gap X (px)", &state.gapX);
		changed |= ImGui::InputInt("Gap Y (px)", &state.gapY);
		changed |= ImGui::InputInt("Margin X (px)", &state.marginX);
		changed |= ImGui::InputInt("Margin Y (px)", &state.marginY);
		if (changed)
		{
			state.columns = std::max(1, state.columns);
			state.rows = std::max(1, state.rows);
			state.gapX = std::max(0, state.gapX);
			state.gapY = std::max(0, state.gapY);
			state.marginX = std::max(0, state.marginX);
			state.marginY = std::max(0, state.marginY);
			MarkSpriteSheetGeneratorDirty();
		}

		if (state.builtRevision != state.dirtyRevision)
			RebuildSpriteSheetGeneratorPreview();

		ImGui::Separator();
		ImGui::TextUnformatted("Preview");
		ImGui::Text("Sheet: %dx%d  Cell: %dx%d  Rows: %d  Used Sprites: %d",
					state.preview.width,
					state.preview.height,
					state.preview.cellWidth,
					state.preview.cellHeight,
					state.preview.usedRows,
					state.preview.placedCount);

		if (state.previewTexture != 0u && state.preview.width > 0 && state.preview.height > 0)
		{
			const float maxWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x);
			const float scale = std::min(1.0f, maxWidth / static_cast<float>(state.preview.width));
			const ImVec2 imageSize(
				std::max(1.0f, static_cast<float>(state.preview.width) * scale),
				std::max(1.0f, static_cast<float>(state.preview.height) * scale));
			ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(state.previewTexture)),
						 imageSize,
						 ImVec2(0, 0),
						 ImVec2(1, 1));
		}
		else
		{
			ImGui::TextUnformatted("Sin preview.");
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Generate");
		ImGui::InputText("Output Path", state.outputPathBuf, sizeof(state.outputPathBuf));
		if (ImGui::Button("Generate SpriteSheet"))
		{
			SpriteSheetComposite composite;
			std::string composeError;
			if (!ComposeSpriteSheet(state, composite, composeError))
			{
				state.status = composeError;
			}
			else
			{
				const std::string outputRaw = TrimCopy(state.outputPathBuf);
				if (outputRaw.empty())
				{
					state.status = "Output Path vacio.";
				}
				else
				{
					std::filesystem::path output = std::filesystem::path(outputRaw).lexically_normal();
					const std::string ext = ToLowerCopy(output.extension().string());
					if (ext != ".tga")
						output.replace_extension(".tga");

					std::string writeError;
					if (WriteTgaRgba(output.string(), composite.width, composite.height, composite.rgba, writeError))
					{
						std::snprintf(state.outputPathBuf, sizeof(state.outputPathBuf), "%s", output.generic_string().c_str());
						state.status = "SpriteSheet generado: " + output.generic_string() +
									   " | Animator: columns=" + std::to_string(std::max(1, state.columns)) +
									   ", rows=" + std::to_string(std::max(1, state.rows)) +
									   ", gapX=" + std::to_string(std::max(0, state.gapX)) +
									   ", gapY=" + std::to_string(std::max(0, state.gapY)) +
									   ", marginX=" + std::to_string(std::max(0, state.marginX)) +
									   ", marginY=" + std::to_string(std::max(0, state.marginY)) +
									   ", cellWidth=" + std::to_string(composite.cellWidth) +
									   ", cellHeight=" + std::to_string(composite.cellHeight) + ".";
					}
					else
					{
						state.status = writeError;
					}
				}
			}
		}

		if (!state.status.empty())
			ImGui::TextWrapped("%s", state.status.c_str());

		ImGui::End();
	}

	static bool IsEditable3DScene(IEditorScene *editorScene)
	{
		return editorScene && editorScene->GetEditorSceneDimension() == EditorSceneDimension::Scene3D;
	}

	static bool HasInputDebugPanelMarker(IEditorScene *editorScene)
	{
		if (!editorScene)
			return false;

		Registry &registry = editorScene->GetEditorRegistry();
		std::vector<Entity> taggedEntities;
		registry.ViewEntities<Tag>(taggedEntities);
		for (Entity e : taggedEntities)
		{
			const Tag &tag = registry.Get<Tag>(e);
			if (tag.name == "InputDebugPanel")
				return true;
		}

		return false;
	}

	static void DrawInputActionDebugPanel(IEditorScene *editorScene)
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

	static Entity GetAliveParent(Registry &r, Entity e)
	{
		if (!r.Has<Parent>(e))
			return kInvalidEntity;
		Entity p = r.Get<Parent>(e).parent;
		return (p != kInvalidEntity && r.IsAlive(p)) ? p : kInvalidEntity;
	}

	struct ResolvedTransform
	{
		glm::vec3 position{0.f, 0.f, 0.f};
		glm::vec3 rotation{0.f, 0.f, 0.f};
		glm::vec3 scale{1.f, 1.f, 1.f};
		glm::mat4 matrix{1.f};
	};

	static glm::mat4 BuildMatrixFromTRS(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
	{
		const glm::mat4 T = glm::translate(glm::mat4(1.f), position);
		const glm::mat4 R =
			glm::rotate(glm::mat4(1.f), rotation.x, glm::vec3(1.f, 0.f, 0.f)) *
			glm::rotate(glm::mat4(1.f), rotation.y, glm::vec3(0.f, 1.f, 0.f)) *
			glm::rotate(glm::mat4(1.f), rotation.z, glm::vec3(0.f, 0.f, 1.f));
		const glm::mat4 S = glm::scale(glm::mat4(1.f), scale);
		return T * R * S;
	}

	static ResolvedTransform ComputeWorldTransform(Registry &r, Entity e)
	{
		ResolvedTransform resolved;
		std::vector<Entity> chain;

		Entity cur = e;
		for (int i = 0; i < 256 && cur != kInvalidEntity && r.IsAlive(cur); ++i)
		{
			chain.push_back(cur);
			const Entity parent = GetAliveParent(r, cur);
			if (parent == kInvalidEntity || !r.Has<Transform>(parent))
				break;
			cur = parent;
		}

		for (auto it = chain.rbegin(); it != chain.rend(); ++it)
		{
			const Entity node = *it;
			if (!r.Has<Transform>(node))
				continue;

			const Transform &local = r.Get<Transform>(node);
			resolved.position = local.absolutePosition ? local.localPosition : (resolved.position + local.localPosition);
			resolved.rotation = local.absoluteRotation ? local.localRotation : (resolved.rotation + local.localRotation);
			resolved.scale = local.absoluteScale ? local.localScale : (resolved.scale * local.localScale);
		}

		resolved.matrix = BuildMatrixFromTRS(resolved.position, resolved.rotation, resolved.scale);
		return resolved;
	}

	static glm::mat4 ComputeWorldTransformMatrix(Registry &r, Entity e)
	{
		return ComputeWorldTransform(r, e).matrix;
	}

	static bool ProjectWorldToViewport(
		const glm::vec3 &worldPos,
		const glm::mat4 &view,
		const glm::mat4 &projection,
		const ImVec2 &rectMin,
		const ImVec2 &rectSize,
		ImVec2 &outScreenPos)
	{
		const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);
		if (clip.w <= 1e-6f)
			return false;

		const glm::vec3 ndc = glm::vec3(clip) / clip.w;
		if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y) || !std::isfinite(ndc.z))
			return false;

		const float u = ndc.x * 0.5f + 0.5f;
		const float v = ndc.y * 0.5f + 0.5f;

		outScreenPos.x = rectMin.x + u * rectSize.x;
		outScreenPos.y = rectMin.y + (1.0f - v) * rectSize.y;
		return true;
	}

	static void DrawSelectedForwardArrow(
		const glm::mat4 &world,
		const glm::mat4 &view,
		const glm::mat4 &projection,
		const ImVec2 &rectMin,
		const ImVec2 &rectSize)
	{
		const glm::vec3 origin = glm::vec3(world[3]);
		glm::vec3 forward = glm::vec3(world * glm::vec4(0.f, 0.f, -1.f, 0.f));
		const float forwardLen = glm::length(forward);
		if (forwardLen <= 1e-6f)
			return;
		forward /= forwardLen;

		ImVec2 p0, pDirSample;
		if (!ProjectWorldToViewport(origin, view, projection, rectMin, rectSize, p0) ||
			!ProjectWorldToViewport(origin + forward, view, projection, rectMin, rectSize, pDirSample))
		{
			return;
		}

		const ImVec2 d{pDirSample.x - p0.x, pDirSample.y - p0.y};
		const float dLen = std::sqrt(d.x * d.x + d.y * d.y);
		if (dLen <= 1e-4f)
			return;

		const ImVec2 dir{d.x / dLen, d.y / dLen};
		const ImVec2 perp{-dir.y, dir.x};

		const float arrowPxLength = 56.0f;
		const ImVec2 p1{p0.x + dir.x * arrowPxLength, p0.y + dir.y * arrowPxLength};

		const float headLen = 12.0f;
		const float headWidth = 5.5f;
		const ImVec2 headBase{p1.x - dir.x * headLen, p1.y - dir.y * headLen};
		const ImVec2 left{headBase.x + perp.x * headWidth, headBase.y + perp.y * headWidth};
		const ImVec2 right{headBase.x - perp.x * headWidth, headBase.y - perp.y * headWidth};

		ImDrawList *dl = ImGui::GetWindowDrawList();
		const ImU32 color = IM_COL32(255, 210, 0, 255);
		dl->AddLine(p0, p1, color, 2.2f);
		dl->AddTriangleFilled(p1, left, right, color);
		dl->AddCircleFilled(p0, 2.5f, color);
	}

	static bool BuildViewportViewProjection(IEditorScene *editorScene,
											const ImVec2 &rectSize,
											Registry *&outRegistry,
											glm::mat4 &outView,
											glm::mat4 &outProjection)
	{
		outRegistry = nullptr;
		if (!editorScene || rectSize.x <= 0.0f || rectSize.y <= 0.0f)
			return false;

		Registry &registry = editorScene->GetEditorRegistry();
		const Entity cameraEntity = FindEditorCameraEntity(registry);
		if (cameraEntity == kInvalidEntity || !registry.Has<Camera>(cameraEntity))
			return false;

		const Camera &camera = registry.Get<Camera>(cameraEntity);
		const float aspect = rectSize.x / rectSize.y;
		outView = glm::lookAt(
			camera.position,
			camera.position + camera.direction,
			glm::vec3(0.f, 1.f, 0.f));

		if (camera.projection == ProjectionType::Orthographic)
		{
			const float halfHeight = camera.orthoHeight * 0.5f;
			const float halfWidth = halfHeight * aspect;
			outProjection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, camera.nearPlane, camera.farPlane);
		}
		else
		{
			outProjection = glm::perspective(glm::radians(camera.fovDeg), aspect, camera.nearPlane, camera.farPlane);
		}

		outRegistry = &registry;
		return true;
	}

	static void DrawCollisionGizmos(IEditorScene *editorScene, const ImVec2 &rectMin, const ImVec2 &rectSize)
	{
		if (!g_showCollisionGizmos)
			return;

		Registry *registry = nullptr;
		glm::mat4 view(1.f);
		glm::mat4 projection(1.f);
		if (!BuildViewportViewProjection(editorScene, rectSize, registry, view, projection))
			return;

		std::vector<Entity> entities;
		registry->ViewEntities<Transform, Collider2D>(entities);
		if (entities.empty())
			return;

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		constexpr int circleSegments = 28;

		for (Entity entity : entities)
		{
			const Transform &transform = registry->Get<Transform>(entity);
			const Collider2D &collider = registry->Get<Collider2D>(entity);
			const bool hasBodyEnabled = !registry->Has<PhysicsBody2D>(entity) || registry->Get<PhysicsBody2D>(entity).enabled;

			ImU32 color = collider.isTrigger
							  ? IM_COL32(255, 170, 0, 220)
							  : IM_COL32(64, 220, 90, 220);
			if (!hasBodyEnabled)
				color = IM_COL32(145, 145, 145, 170);
			else if (registry->Has<RigidBody2D>(entity))
			{
				const RigidBody2D &body = registry->Get<RigidBody2D>(entity);
				const bool dynamicBody = !body.isKinematic && body.mass > 0.0f;
				if (dynamicBody)
					color = collider.isTrigger ? IM_COL32(255, 170, 0, 220) : IM_COL32(0, 200, 255, 220);
			}

			const Physics2DAlgorithms::WorldShape2D shape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
			const float z = transform.localPosition.z;

			if (shape.shape == Collider2D::Shape::AABB)
			{
				const glm::vec2 ex = shape.axisX * shape.halfExtents.x;
				const glm::vec2 ey = shape.axisY * shape.halfExtents.y;
				ImVec2 points[4];
				const bool allProjected =
					ProjectWorldToViewport(glm::vec3(shape.center - ex - ey, z), view, projection, rectMin, rectSize, points[0]) &&
					ProjectWorldToViewport(glm::vec3(shape.center + ex - ey, z), view, projection, rectMin, rectSize, points[1]) &&
					ProjectWorldToViewport(glm::vec3(shape.center + ex + ey, z), view, projection, rectMin, rectSize, points[2]) &&
					ProjectWorldToViewport(glm::vec3(shape.center - ex + ey, z), view, projection, rectMin, rectSize, points[3]);
				if (!allProjected)
					continue;

				drawList->AddPolyline(points, 4, color, ImDrawFlags_Closed, 1.5f);
			}
			else
			{
				ImVec2 points[circleSegments];
				bool allProjected = true;
				for (int i = 0; i < circleSegments; ++i)
				{
					const float angle = (6.28318530718f * static_cast<float>(i)) / static_cast<float>(circleSegments);
					const glm::vec2 unit(std::cos(angle), std::sin(angle));
					const glm::vec2 point = shape.center + unit * shape.radius;
					if (!ProjectWorldToViewport(glm::vec3(point.x, point.y, z), view, projection, rectMin, rectSize, points[i]))
					{
						allProjected = false;
						break;
					}
				}
				if (!allProjected)
					continue;

				drawList->AddPolyline(points, circleSegments, color, ImDrawFlags_Closed, 1.5f);
			}
		}
	}

	static Transform DecomposeTransform(const glm::mat4 &matrix, const Transform &keepPivot)
	{
		Transform out = keepPivot;
		float m[16];
		std::memcpy(m, glm::value_ptr(matrix), sizeof(m));

		float translation[3] = {0.f, 0.f, 0.f};
		float rotationDeg[3] = {0.f, 0.f, 0.f};
		float scale[3] = {1.f, 1.f, 1.f};
		ImGuizmo::DecomposeMatrixToComponents(m, translation, rotationDeg, scale);

		out.localPosition = glm::vec3(translation[0], translation[1], translation[2]);
		out.localRotation = glm::radians(glm::vec3(rotationDeg[0], rotationDeg[1], rotationDeg[2]));
		out.localScale = glm::vec3(scale[0], scale[1], scale[2]);
		return out;
	}

	static glm::vec3 DivideScaleSafe(const glm::vec3 &lhs, const glm::vec3 &rhs)
	{
		auto divide = [](float left, float right)
		{
			return (std::abs(right) > 1e-6f) ? (left / right) : left;
		};
		return glm::vec3{
			divide(lhs.x, rhs.x),
			divide(lhs.y, rhs.y),
			divide(lhs.z, rhs.z)};
	}

	static Transform ConvertWorldTransformToLocal(const Transform &currentLocal,
												  const Transform &worldTransform,
												  const ResolvedTransform &parentWorld,
												  bool hasParent)
	{
		Transform out = currentLocal;

		out.localPosition =
			(currentLocal.absolutePosition || !hasParent)
				? worldTransform.localPosition
				: (worldTransform.localPosition - parentWorld.position);

		out.localRotation =
			(currentLocal.absoluteRotation || !hasParent)
				? worldTransform.localRotation
				: (worldTransform.localRotation - parentWorld.rotation);

		out.localScale =
			(currentLocal.absoluteScale || !hasParent)
				? worldTransform.localScale
				: DivideScaleSafe(worldTransform.localScale, parentWorld.scale);

		return out;
	}

	static bool TransformEqualEpsilon(const Transform &a, const Transform &b)
	{
		constexpr float eps = 1e-4f;
		return glm::all(glm::lessThanEqual(glm::abs(a.localPosition - b.localPosition), glm::vec3(eps))) &&
			   glm::all(glm::lessThanEqual(glm::abs(a.localRotation - b.localRotation), glm::vec3(eps))) &&
			   glm::all(glm::lessThanEqual(glm::abs(a.localScale - b.localScale), glm::vec3(eps))) &&
			   glm::all(glm::lessThanEqual(glm::abs(a.pivot - b.pivot), glm::vec3(eps))) &&
			   a.absolutePosition == b.absolutePosition &&
			   a.absoluteRotation == b.absoluteRotation &&
			   a.absoluteScale == b.absoluteScale;
	}

	static Entity FindEditorCameraEntity(Registry &r)
	{
		std::vector<Entity> cameras;
		r.ViewEntities<Camera>(cameras);

		if (cameras.empty())
			return kInvalidEntity;

		for (Entity e : cameras)
		{
			if (r.Has<CameraController>(e))
				return e;
		}
		return cameras.front();
	}

	static void PushTransformCommand(TransformHistory &history, const TransformEditCommand &command)
	{
		history.undoStack.push_back(command);
		history.redoStack.clear();
	}

	static bool ApplyTransformCommand(Registry &r, const TransformEditCommand &command, bool applyAfter)
	{
		if (command.entity == kInvalidEntity || !r.IsAlive(command.entity) || !r.Has<Transform>(command.entity))
			return false;

		r.Get<Transform>(command.entity) = applyAfter ? command.after : command.before;
		return true;
	}

	static bool PerformUndo(Registry &r, TransformHistory &history)
	{
		if (history.undoStack.empty())
			return false;

		const TransformEditCommand command = history.undoStack.back();
		history.undoStack.pop_back();
		if (!ApplyTransformCommand(r, command, false))
			return false;

		history.redoStack.push_back(command);
		return true;
	}

	static bool PerformRedo(Registry &r, TransformHistory &history)
	{
		if (history.redoStack.empty())
			return false;

		const TransformEditCommand command = history.redoStack.back();
		history.redoStack.pop_back();
		if (!ApplyTransformCommand(r, command, true))
			return false;

		history.undoStack.push_back(command);
		return true;
	}

	static void ClearGizmoRigidBodyOverride()
	{
		g_gizmoRigidBodyOverride.scene = nullptr;
		g_gizmoRigidBodyOverride.entity = kInvalidEntity;
		g_gizmoRigidBodyOverride.hasOverride = false;
		g_gizmoRigidBodyOverride.originalIsKinematic = false;
	}

	static void ApplyGizmoRigidBodyOverride(IEditorScene *scene, Registry &registry, Entity entity)
	{
		if (!scene || entity == kInvalidEntity || !registry.IsAlive(entity) || !registry.Has<RigidBody2D>(entity))
			return;

		if (g_gizmoRigidBodyOverride.hasOverride &&
			g_gizmoRigidBodyOverride.scene == scene &&
			g_gizmoRigidBodyOverride.entity == entity)
		{
			registry.Get<RigidBody2D>(entity).isKinematic = true;
			return;
		}

		if (g_gizmoRigidBodyOverride.hasOverride &&
			g_gizmoRigidBodyOverride.scene &&
			g_gizmoRigidBodyOverride.entity != kInvalidEntity)
		{
			Registry &prevRegistry = g_gizmoRigidBodyOverride.scene->GetEditorRegistry();
			if (prevRegistry.IsAlive(g_gizmoRigidBodyOverride.entity) &&
				prevRegistry.Has<RigidBody2D>(g_gizmoRigidBodyOverride.entity))
			{
				prevRegistry.Get<RigidBody2D>(g_gizmoRigidBodyOverride.entity).isKinematic =
					g_gizmoRigidBodyOverride.originalIsKinematic;
			}
			ClearGizmoRigidBodyOverride();
		}

		RigidBody2D &body = registry.Get<RigidBody2D>(entity);
		g_gizmoRigidBodyOverride.scene = scene;
		g_gizmoRigidBodyOverride.entity = entity;
		g_gizmoRigidBodyOverride.hasOverride = true;
		g_gizmoRigidBodyOverride.originalIsKinematic = body.isKinematic;
		body.isKinematic = true;
	}

	static void RestoreGizmoRigidBodyOverride()
	{
		if (!g_gizmoRigidBodyOverride.hasOverride || !g_gizmoRigidBodyOverride.scene)
			return;

		Registry &registry = g_gizmoRigidBodyOverride.scene->GetEditorRegistry();
		if (registry.IsAlive(g_gizmoRigidBodyOverride.entity) &&
			registry.Has<RigidBody2D>(g_gizmoRigidBodyOverride.entity))
		{
			registry.Get<RigidBody2D>(g_gizmoRigidBodyOverride.entity).isKinematic =
				g_gizmoRigidBodyOverride.originalIsKinematic;
		}

		ClearGizmoRigidBodyOverride();
	}

	static void DrawGizmoToolbar(bool viewportFocused)
	{
		ImGuiIO &io = ImGui::GetIO();
		if (viewportFocused && !io.WantTextInput)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_W))
				g_gizmoState.operation = ImGuizmo::TRANSLATE;
			else if (ImGui::IsKeyPressed(ImGuiKey_E))
				g_gizmoState.operation = ImGuizmo::ROTATE;
			else if (ImGui::IsKeyPressed(ImGuiKey_R))
				g_gizmoState.operation = ImGuizmo::SCALE;
		}

		if (ImGui::RadioButton("Translate (W)", g_gizmoState.operation == ImGuizmo::TRANSLATE))
			g_gizmoState.operation = ImGuizmo::TRANSLATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate (E)", g_gizmoState.operation == ImGuizmo::ROTATE))
			g_gizmoState.operation = ImGuizmo::ROTATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale (R)", g_gizmoState.operation == ImGuizmo::SCALE))
			g_gizmoState.operation = ImGuizmo::SCALE;

		ImGui::Separator();

		if (ImGui::RadioButton("Local", g_gizmoState.mode == ImGuizmo::LOCAL))
			g_gizmoState.mode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", g_gizmoState.mode == ImGuizmo::WORLD))
			g_gizmoState.mode = ImGuizmo::WORLD;
	}

	static void DrawTransformGizmo(IEditorScene *editorScene, SceneEditorState &se, const ImVec2 &rectMin, const ImVec2 &rectSize, bool viewportFocused, bool imageHovered)
	{
		g_gizmoRuntimeDebug = {};

		auto finalizeManipulation = [&](Registry &r)
		{
			if (!g_gizmoState.isManipulating)
				return;

			g_gizmoState.isManipulating = false;

			if (g_gizmoState.activeScene == editorScene &&
				g_gizmoState.activeEntity != kInvalidEntity &&
				r.IsAlive(g_gizmoState.activeEntity) &&
				r.Has<Transform>(g_gizmoState.activeEntity))
			{
				const Transform endTransform = r.Get<Transform>(g_gizmoState.activeEntity);
				if (!TransformEqualEpsilon(g_gizmoState.beginTransform, endTransform))
				{
					TransformHistory &history = g_transformHistoryByScene[editorScene];
					PushTransformCommand(history, TransformEditCommand{
													  g_gizmoState.activeEntity,
													  g_gizmoState.beginTransform,
													  endTransform});
				}
			}

			g_gizmoState.activeScene = nullptr;
			g_gizmoState.activeEntity = kInvalidEntity;
			g_gizmoState.hasActiveWorldMatrix = false;
			RestoreGizmoRigidBodyOverride();
		};

		if (!editorScene || rectSize.x <= 0.0f || rectSize.y <= 0.0f)
		{
			RestoreGizmoRigidBodyOverride();
			g_gizmoState.isManipulating = false;
			g_gizmoState.activeScene = nullptr;
			g_gizmoState.activeEntity = kInvalidEntity;
			g_gizmoState.hasActiveWorldMatrix = false;
			return;
		}

		Registry &r = editorScene->GetEditorRegistry();
		Entity e = se.selectedEntity;
		if (e == kInvalidEntity || !r.IsAlive(e) || !r.Has<Transform>(e))
		{
			finalizeManipulation(r);
			RestoreGizmoRigidBodyOverride();
			return;
		}
		g_gizmoRuntimeDebug.hasTarget = true;

		const Entity cameraEntity = FindEditorCameraEntity(r);
		if (cameraEntity == kInvalidEntity || !r.Has<Camera>(cameraEntity))
		{
			finalizeManipulation(r);
			RestoreGizmoRigidBodyOverride();
			return;
		}

		const Camera &cam = r.Get<Camera>(cameraEntity);
		const float aspect = (rectSize.y <= 0.0f) ? 1.0f : (rectSize.x / rectSize.y);

		const glm::mat4 view = glm::lookAt(
			cam.position,
			cam.position + cam.direction,
			glm::vec3(0.f, 1.f, 0.f));

		glm::mat4 projection(1.0f);
		if (cam.projection == ProjectionType::Orthographic)
		{
			const float halfHeight = cam.orthoHeight * 0.5f;
			const float halfWidth = halfHeight * aspect;
			projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, cam.nearPlane, cam.farPlane);
		}
		else
		{
			projection = glm::perspective(glm::radians(cam.fovDeg), aspect, cam.nearPlane, cam.farPlane);
		}

		Transform &localTransform = r.Get<Transform>(e);
		const Transform transformBeforeFrame = localTransform;

		const Entity parent = GetAliveParent(r, e);
		const bool hasParent = (parent != kInvalidEntity && r.Has<Transform>(parent));
		const ResolvedTransform parentWorld = hasParent ? ComputeWorldTransform(r, parent) : ResolvedTransform{};
		const glm::mat4 world = ComputeWorldTransformMatrix(r, e);

		glm::mat4 manipulatedWorld = world;
		const bool hasActiveManipulationMatrix =
			g_gizmoState.isManipulating &&
			g_gizmoState.activeScene == editorScene &&
			g_gizmoState.activeEntity == e &&
			g_gizmoState.hasActiveWorldMatrix;
		if (hasActiveManipulationMatrix)
			manipulatedWorld = g_gizmoState.activeWorldMatrix;

		ImGuizmo::SetOrthographic(cam.projection == ProjectionType::Orthographic);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);
		const bool gizmoEnabled = viewportFocused && imageHovered;
		ImGuizmo::Enable(gizmoEnabled);
		g_gizmoRuntimeDebug.enabled = gizmoEnabled;

		(void)ImGuizmo::Manipulate(
			glm::value_ptr(view),
			glm::value_ptr(projection),
			g_gizmoState.operation,
			g_gizmoState.mode,
			glm::value_ptr(manipulatedWorld));

		const bool usingNow = ImGuizmo::IsUsing();
		g_gizmoRuntimeDebug.isUsing = usingNow;
		g_gizmoRuntimeDebug.isOver = ImGuizmo::IsOver();

		if (usingNow)
		{
			ApplyGizmoRigidBodyOverride(editorScene, r, e);

			if (!g_gizmoState.isManipulating)
			{
				g_gizmoState.isManipulating = true;
				g_gizmoState.activeScene = editorScene;
				g_gizmoState.activeEntity = e;
				g_gizmoState.beginTransform = transformBeforeFrame;
				g_gizmoState.activeWorldMatrix = world;
				g_gizmoState.hasActiveWorldMatrix = true;
			}
			g_gizmoState.activeWorldMatrix = manipulatedWorld;
			g_gizmoState.hasActiveWorldMatrix = true;

			const Transform worldTransform = DecomposeTransform(g_gizmoState.activeWorldMatrix, localTransform);
			Transform updatedTransform = ConvertWorldTransformToLocal(localTransform, worldTransform, parentWorld, hasParent);
			if (g_gizmoState.operation == ImGuizmo::TRANSLATE)
			{
				updatedTransform.localRotation = transformBeforeFrame.localRotation;
				updatedTransform.localScale = transformBeforeFrame.localScale;
			}
			else if (g_gizmoState.operation == ImGuizmo::SCALE)
			{
				updatedTransform.localRotation = transformBeforeFrame.localRotation;
			}
			else if (g_gizmoState.operation == ImGuizmo::ROTATE)
			{
				updatedTransform.localPosition = transformBeforeFrame.localPosition;
				updatedTransform.localScale = transformBeforeFrame.localScale;
			}

			localTransform = updatedTransform;
		}
		else if (g_gizmoState.isManipulating)
		{
			finalizeManipulation(r);
		}
		else
		{
			RestoreGizmoRigidBodyOverride();
		}

		if (g_showForwardArrow && r.IsAlive(e) && !r.Has<CameraController>(e))
		{
			const glm::mat4 arrowWorld =
				(usingNow && g_gizmoState.hasActiveWorldMatrix)
					? g_gizmoState.activeWorldMatrix
					: ComputeWorldTransformMatrix(r, e);
			DrawSelectedForwardArrow(arrowWorld, view, projection, rectMin, rectSize);
		}
	}

	static Entity AddBasicShape(IEditorScene *editorScene, SceneEditorState &se, const std::string &name, const std::string &meshPath)
	{
		if (!editorScene)
			return kInvalidEntity;

		Registry &r = editorScene->GetEditorRegistry();
		Entity e = SceneEditor::CreateEntity(r, name.c_str(), kInvalidEntity, true);

		if (!r.Has<Mesh>(e))
			r.Emplace<Mesh>(e);

		auto &mesh = r.Get<Mesh>(e);
		mesh.meshPath = meshPath;
		mesh.materialPath = "res/shaders/lit3D.fs";

		if (!r.Has<Material>(e))
			r.Emplace<Material>(e);

		std::string err;
		if (!editorScene->EditorSetMeshPath(e, mesh.meshPath, err))
		{
			se.inspectorStatus = "Basic shape mesh error: " + err;
		}

		if (!editorScene->EditorSetMeshMaterial(e, mesh.materialPath, err))
			se.inspectorStatus = "Basic shape material error: " + err;
		else if (se.inspectorStatus.rfind("Basic shape mesh error:", 0) != 0)
			se.inspectorStatus.clear();

		return e;
	}

	static Transform BuildPrefabSpawnTransform(IEditorScene *editorScene, const SceneEditorState &se)
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

	static Entity AddPrefab(IEditorScene *editorScene,
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

	static void AddSystemPopup(IEditorScene *editorScene)
	{
		if (!editorScene)
			return;

		if (!ImGui::BeginPopup("AddSystemPopup"))
			return;

		static char filter[128] = {};
		ImGui::InputText("Search", filter, sizeof(filter));

		auto pass = [&](const char *name)
		{
			if (filter[0] == 0)
				return true;
			return std::string(name).find(filter) != std::string::npos;
		};

		std::vector<EditorSystemToggle> systems;
		editorScene->GetEditorSystems(systems);
		auto &removedSystems = g_removedSystemsByScene[editorScene];

		bool anyAddable = false;
		for (auto &sys : systems)
		{
			const bool isRemoved = removedSystems.find(sys.name) != removedSystems.end();
			const bool isEnabled = (sys.enabled && *sys.enabled);
			if (!isRemoved && isEnabled)
				continue;
			if (!pass(sys.name))
				continue;

			anyAddable = true;
			if (ImGui::MenuItem(sys.name))
			{
				if (sys.enabled)
					*sys.enabled = true;
				removedSystems.erase(sys.name);
			}
		}

		if (!anyAddable)
			ImGui::TextUnformatted("No systems available to add.");

		ImGui::EndPopup();
	}

	static void DrawSceneFilePopups(SceneManager &scenes, EditorUIState &ui)
	{
		static std::vector<std::string> saveSceneEntries;
		static char saveSceneFilter[128] = {};
		static int saveSceneSelection = -1;
		static bool refreshSaveSceneList = false;
		static std::vector<std::string> openSceneEntries;
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
				saveSceneEntries = CollectSceneAssetEntries();
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
					const std::string lower = ToLowerCopy(path);
					if (lower.find(filter) == std::string::npos)
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
				openSceneEntries = CollectSceneAssetEntries();
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
					const std::string lower = ToLowerCopy(path);
					if (lower.find(filter) == std::string::npos)
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

	static void DrawTopBar(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *es)
	{
		if (!std::isfinite(ui.dragSnapStep) || ui.dragSnapStep <= 0.0f)
			ui.dragSnapStep = 0.5f;

		if (!ImGui::BeginMainMenuBar())
			return;

		DrawSceneFilePopups(scenes, ui);

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save Selected Scene..."))
				ui.saveScenePopupOpen = true;
			if (ImGui::MenuItem("Open Scene From Disk..."))
				ui.openScenePopupOpen = true;
			ImGui::EndMenu();
		}

		bool canEdit = (es != nullptr);

		if (ImGui::BeginMenu("Edit"))
		{
			TransformHistory *history = nullptr;
			if (es)
			{
				history = &g_transformHistoryByScene[es];
			}

			const bool canUndo = history && !history->undoStack.empty();
			const bool canRedo = history && !history->redoStack.empty();

			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
			{
				Registry &r = es->GetEditorRegistry();
				(void)PerformUndo(r, *history);
			}

			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
			{
				Registry &r = es->GetEditorRegistry();
				(void)PerformRedo(r, *history);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Duplicate", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
			{
				Registry &r = es->GetEditorRegistry();
				se.selectedEntity = SceneEditor::DuplicateEntityRecursive(r, se.selectedEntity);
			}

			if (ImGui::MenuItem("Delete", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
			{
				Registry &r = es->GetEditorRegistry();
				SceneEditor::DestroyEntityRecursive(r, se.selectedEntity);
				se.selectedEntity = kInvalidEntity;
			}

			if (ImGui::MenuItem("Rename", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
				SceneEditor::BeginRename(es->GetEditorRegistry(), se, se.selectedEntity);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("SceneList", nullptr, &ui.showSceneList);
			ImGui::MenuItem("Entity Hierarchy", nullptr, &ui.showEntityHierarchy);
			ImGui::MenuItem("Systems", nullptr, &ui.showSystems);
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
			ImGui::MenuItem("Debug Clicks", nullptr, &g_showGizmoDebug);
			ImGui::MenuItem("Forward Arrow", nullptr, &g_showForwardArrow);
			if (ImGui::BeginMenu("Physics2D"))
			{
				if (es)
				{
					glm::vec2 gravity = es->GetPhysics2DGravity();
					ImGui::SetNextItemWidth(170.0f);
					if (ImGui::DragFloat2("Gravity2D", &gravity.x, 0.05f, -1000.0f, 1000.0f, "%.3f"))
						es->SetPhysics2DGravity(gravity);

					ImGui::MenuItem("Show 2DCollisions Gizmos", nullptr, &g_showCollisionGizmos);
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
				Registry &r = es->GetEditorRegistry();
				se.selectedEntity = SceneEditor::CreateEntity(r, "New Entity", kInvalidEntity, true);
			}

			if (ImGui::MenuItem("Child Entity", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
			{
				Registry &r = es->GetEditorRegistry();
				se.selectedEntity = SceneEditor::CreateEntity(r, "New Entity", se.selectedEntity, true);
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
					es->GetPrefabRegistry().GetNames(prefabNames);
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
								se.selectedEntity = AddPrefab(es, se, prefabName);
							ImGui::PopID();
						}
					}

					if (es->GetPrefabRegistry().Has("EnemyBasic"))
					{
						ImGui::Separator();
						if (ImGui::MenuItem("EnemyBasic (Override Tint/Size)"))
						{
							prefab::PrefabSpawnOverrides overrides;
							overrides.spriteTint = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
							overrides.spriteSize = glm::vec2(1.4f, 1.4f);
							se.selectedEntity = AddPrefab(es, se, "EnemyBasic", &overrides);
						}
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Add Basic Shape"))
			{
				const bool canAddShape = canEdit && IsEditable3DScene(es);
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
								se.selectedEntity = AddBasicShape(es, se, shape.label, shape.meshPath);
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
			if (ImGui::MenuItem("SpriteSheet Generator"))
				g_spriteSheetGenerator.open = true;
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

	void Editor::Draw(SceneManager &scenes, EditorUIState &state)
	{
		ImGuizmo::BeginFrame();
		DrawDockSpace(state);
		g_renderWindowHovered = false;

		const size_t count = scenes.GetLoadedSceneCount();
		if (state.selectedScene >= count)
			state.selectedScene = 0;

		auto scene = scenes.GetLoadedScene(state.selectedScene);
		IEditorScene *editorScene = scene ? dynamic_cast<IEditorScene *>(scene.get()) : nullptr;
		scenes.SetEditorSelectedSceneIndex(editorScene ? state.selectedScene : static_cast<size_t>(-1));

		static SceneEditorState se;

		DrawTopBar(scenes, state, se, editorScene);
		DrawSpriteSheetGeneratorWindow();
		SceneEditor::SetDragSnapStep(state.dragSnapStep);

		if (editorScene)
		{
			ImGuiIO &io = ImGui::GetIO();
			if (!io.WantTextInput && !g_gizmoState.isManipulating && io.KeyCtrl)
			{
				TransformHistory &history = g_transformHistoryByScene[editorScene];
				Registry &r = editorScene->GetEditorRegistry();

				if (ImGui::IsKeyPressed(ImGuiKey_Z))
				{
					if (io.KeyShift)
						(void)PerformRedo(r, history);
					else
						(void)PerformUndo(r, history);
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Y))
				{
					(void)PerformRedo(r, history);
				}
			}
		}

		if (state.showSceneList)
		{
			ImGui::Begin("SceneList");

			if (ImGui::Button("Clear Non-Core"))
			{
				scenes.RequestClearNonCore();
				state.selectedScene = 0;
				se.selectedEntity = kInvalidEntity;
			}

			ImGui::Separator();

			if (editorScene)
			{
				int sceneDimIndex = (editorScene->GetEditorSceneDimension() == EditorSceneDimension::Scene3D) ? 0 : 1;
				if (ImGui::Combo("Scene Type", &sceneDimIndex, "3D\0"
															   "2D\0"))
				{
					const EditorSceneDimension newDim = (sceneDimIndex == 0) ? EditorSceneDimension::Scene3D : EditorSceneDimension::Scene2D;
					editorScene->SetEditorSceneDimension(newDim);
					se.selectedEntity = kInvalidEntity;
					g_removedSystemsByScene[editorScene].clear();
				}

				if (sceneDimIndex == 0)
					ImGui::TextUnformatted("Camera 3D: RMB rota, WASD mueve, Q/E sube-baja.");
				else
					ImGui::TextUnformatted("Camera 2D: RMB arrastra para pan XY, WASD mueve XY.");

				ImGui::Separator();
			}

			for (size_t i = 0; i < count; ++i)
			{
				const char *name = scenes.GetLoadedSceneName(i);

				ImGui::PushID((int)i);

				bool sel = (state.selectedScene == i);
				const bool renaming = (state.renamingSceneIndex == i);
				const bool canRename = true;
				const bool canDelete = (i != 0);
				ImVec2 selectableSize(0.0f, 0.0f);
				if (!renaming)
				{
					const ImGuiStyle &style = ImGui::GetStyle();
					const float renameButtonWidth = canRename ? (ImGui::CalcTextSize("Rename").x + style.FramePadding.x * 2.0f) : 0.0f;
					const float deleteButtonWidth = ImGui::CalcTextSize("Delete").x + style.FramePadding.x * 2.0f;
					const float spacing = style.ItemSpacing.x * (canDelete ? 2.0f : 1.0f);
					const float availableWidth = ImGui::GetContentRegionAvail().x;
					float buttonWidth = renameButtonWidth;
					if (canDelete)
						buttonWidth += deleteButtonWidth;
					selectableSize.x = ImMax(1.0f, availableWidth - buttonWidth - spacing);
				}

				if (renaming)
				{
					if (ImGui::InputText("##scene_rename", state.renameSceneBuf, sizeof(state.renameSceneBuf), ImGuiInputTextFlags_EnterReturnsTrue))
					{
						std::string newName = TrimCopy(state.renameSceneBuf);
						if (!newName.empty())
							scenes.RenameLoadedScene(i, newName);
						state.renamingSceneIndex = static_cast<size_t>(-1);
					}

					if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
						state.renamingSceneIndex = static_cast<size_t>(-1);
				}
				else if (ImGui::Selectable(name, sel, 0, selectableSize))
				{
					state.selectedScene = i;
					se.selectedEntity = kInvalidEntity;
				}

				if (canRename)
				{
					ImGui::SameLine();
					if (ImGui::SmallButton("Rename"))
					{
						state.renamingSceneIndex = i;
						std::snprintf(state.renameSceneBuf, sizeof(state.renameSceneBuf), "%s", name);
					}
				}

				if (canDelete)
				{
					ImGui::SameLine();
					if (ImGui::SmallButton("Delete"))
					{
						scenes.RequestRemoveLoadedScene(i);
						if (state.selectedScene == i)
						{
							state.selectedScene = 0;
							se.selectedEntity = kInvalidEntity;
						}
					}
				}

				ImGui::PopID();
			}

			ImGui::End();
		}

		if (state.showEntityHierarchy)
		{
			ImGui::Begin("Entity Hierarchy");

			if (!editorScene)
			{
				ImGui::TextUnformatted("Scene is not editor-inspectable.");
				ImGui::End();
			}
			else
			{
				Registry &r = editorScene->GetEditorRegistry();
				SceneEditor::DrawHierarchy(r, se);
				ImGui::End();
			}
		}

		if (state.showSystems)
		{
			ImGui::Begin("Systems");

			if (!editorScene)
			{
				ImGui::TextUnformatted("Scene is not editor-inspectable.");
			}
			else
			{
				if (ImGui::Button("Add System"))
					ImGui::OpenPopup("AddSystemPopup");

				AddSystemPopup(editorScene);

				ImGui::Separator();

				std::vector<EditorSystemToggle> sys;
				editorScene->GetEditorSystems(sys);
				auto &removedSystems = g_removedSystemsByScene[editorScene];

				for (auto &s : sys)
				{
					if (removedSystems.find(s.name) != removedSystems.end())
						continue;

					ImGui::PushID(s.name);
					if (s.enabled)
						ImGui::Checkbox("##enabled", s.enabled);
					else
					{
						bool disabled = false;
						ImGui::BeginDisabled();
						ImGui::Checkbox("##enabled", &disabled);
						ImGui::EndDisabled();
					}
					ImGui::SameLine();
					ImGui::TextUnformatted(s.name);
					ImGui::SameLine();
					if (ImGui::SmallButton("Remove"))
					{
						removedSystems.insert(s.name);
						if (s.enabled)
							*s.enabled = false;
					}
					ImGui::PopID();
				}
			}

			ImGui::End();
		}

		if (state.showInspector)
		{
			ImGui::Begin("Inspector");

			if (!editorScene)
			{
				ImGui::TextUnformatted("Scene is not editor-inspectable.");
			}
			else
			{
				Registry &r = editorScene->GetEditorRegistry();
				SceneEditor::DrawInspector(r, se, editorScene);
			}

			ImGui::End();
		}

		if (state.showRender)
		{
			ImGuiWindowFlags renderFlags =
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoBackground;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Render", &state.showRender, renderFlags);
			g_renderWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

			const bool viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
			ImGui::BeginChild("##RenderToolbar", ImVec2(0.0f, 64.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			DrawGizmoToolbar(viewportFocused);
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImVec2 avail = ImGui::GetContentRegionAvail();
			if (avail.x < 1.0f)
				avail.x = 1.0f;
			if (avail.y < 1.0f)
				avail.y = 1.0f;

			g_renderTargetSize = avail;

			if (g_renderTexture)
			{
				ImGui::Image(g_renderTexture, avail, ImVec2(0, 1), ImVec2(1, 0));
				const ImVec2 imageMin = ImGui::GetItemRectMin();
				const ImVec2 imageSize = ImGui::GetItemRectSize();
				const bool imageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
				DrawCollisionGizmos(editorScene, imageMin, imageSize);
				DrawTransformGizmo(editorScene, se, imageMin, imageSize, viewportFocused, imageHovered);

				if (g_showGizmoDebug)
				{
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
								g_gizmoRuntimeDebug.hasTarget ? 1 : 0,
								g_gizmoRuntimeDebug.enabled ? 1 : 0,
								g_gizmoRuntimeDebug.isOver ? 1 : 0,
								g_gizmoRuntimeDebug.isUsing ? 1 : 0);
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
									g_gizmoRuntimeDebug.hasTarget ? 1 : 0,
									g_gizmoRuntimeDebug.enabled ? 1 : 0,
									g_gizmoRuntimeDebug.isOver ? 1 : 0,
									g_gizmoRuntimeDebug.isUsing ? 1 : 0,
									io.MousePos.x,
									io.MousePos.y,
									hoveredName,
									hoveredId,
									activeId);
					}
				}
			}
			else
			{
				ImGui::TextUnformatted("Render target not ready.");
			}

			ImGui::End();
			ImGui::PopStyleVar();
		}

		DrawInputActionDebugPanel(editorScene);
	}
	void Dockspace::Draw(EditorUIState &state)
	{
		DrawDockSpace(state);
	}

	void SceneFileDialogs::Draw(SceneManager &scenes, EditorUIState &ui)
	{
		DrawSceneFilePopups(scenes, ui);
	}

	void TransformUndoRedoService::DrawShortcutHandlers(IEditorScene *editorScene)
	{
		if (!editorScene)
			return;

		ImGuiIO &io = ImGui::GetIO();
		if (!io.WantTextInput && !g_gizmoState.isManipulating && io.KeyCtrl)
		{
			TransformHistory &history = g_transformHistoryByScene[editorScene];
			Registry &r = editorScene->GetEditorRegistry();

			if (ImGui::IsKeyPressed(ImGuiKey_Z))
			{
				if (io.KeyShift)
					(void)PerformRedo(r, history);
				else
					(void)PerformUndo(r, history);
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_Y))
			{
				(void)PerformRedo(r, history);
			}
		}
	}

	void TransformUndoRedoService::DrawUndoRedoMenuItems(IEditorScene *editorScene)
	{
		TransformHistory *history = nullptr;
		if (editorScene)
			history = &g_transformHistoryByScene[editorScene];

		const bool canUndo = history && !history->undoStack.empty();
		const bool canRedo = history && !history->redoStack.empty();

		if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
		{
			Registry &r = editorScene->GetEditorRegistry();
			(void)PerformUndo(r, *history);
		}

		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
		{
			Registry &r = editorScene->GetEditorRegistry();
			(void)PerformRedo(r, *history);
		}
	}

	void TopBar::Draw(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene)
	{
		DrawTopBar(scenes, ui, se, editorScene);
	}

	void SceneListPanel::Draw(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene)
	{
		(void)se;
		(void)editorScene;
		(void)scenes;
		(void)ui;
	}

	void SystemsPanel::Draw(IEditorScene *editorScene)
	{
		(void)editorScene;
	}

	void HierarchyPanel::Draw(IEditorScene *editorScene, SceneEditorState &se)
	{
		(void)editorScene;
		(void)se;
	}

	void InspectorPanel::Draw(IEditorScene *editorScene, SceneEditorState &se)
	{
		(void)editorScene;
		(void)se;
	}

	void ViewportToolbar::Draw(bool viewportFocused)
	{
		DrawGizmoToolbar(viewportFocused);
	}

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

	void ForwardArrowOverlay::Draw(const glm::mat4 &world,
								   const glm::mat4 &view,
								   const glm::mat4 &projection,
								   const ImVec2 &rectMin,
								   const ImVec2 &rectSize)
	{
		DrawSelectedForwardArrow(world, view, projection, rectMin, rectSize);
	}

	void GizmoDebugOverlay::Draw(bool viewportFocused, bool imageHovered, const ImVec2 &imageMin)
	{
		if (!g_showGizmoDebug)
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
					g_gizmoRuntimeDebug.hasTarget ? 1 : 0,
					g_gizmoRuntimeDebug.enabled ? 1 : 0,
					g_gizmoRuntimeDebug.isOver ? 1 : 0,
					g_gizmoRuntimeDebug.isUsing ? 1 : 0);
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
						g_gizmoRuntimeDebug.hasTarget ? 1 : 0,
						g_gizmoRuntimeDebug.enabled ? 1 : 0,
						g_gizmoRuntimeDebug.isOver ? 1 : 0,
						g_gizmoRuntimeDebug.isUsing ? 1 : 0,
						io.MousePos.x,
						io.MousePos.y,
						hoveredName,
						hoveredId,
						activeId);
		}
	}

	void TransformGizmoController::Draw(IEditorScene *editorScene,
										SceneEditorState &se,
										const ImVec2 &rectMin,
										const ImVec2 &rectSize,
										bool viewportFocused,
										bool imageHovered)
	{
		DrawTransformGizmo(editorScene, se, rectMin, rectSize, viewportFocused, imageHovered);
	}

	void ViewportPanel::SetRenderTexture(ImTextureID textureId)
	{
		g_renderTexture = textureId;
	}

	ImVec2 ViewportPanel::GetRenderTargetSize()
	{
		return g_renderTargetSize;
	}

	bool ViewportPanel::IsRenderWindowHovered()
	{
		return g_renderWindowHovered;
	}

	void ViewportPanel::Draw(EditorUIState &state, IEditorScene *editorScene, SceneEditorState &se)
	{
		(void)state;
		(void)editorScene;
		(void)se;
	}
}
#pragma endregion
