#include "EditorModulesInternal.h"

#include "../input/Input.h"
#include "../platform/GL.h"
#include "../thirdparty/stb_image.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace editorui::internal
{
	namespace
	{
		std::string ResolveRuntimeAssetPath(const std::string &rawPath)
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

		void DestroySpriteSheetPreviewTexture(SpriteSheetGeneratorState &state)
		{
			if (state.previewTexture != 0u)
			{
				GLuint id = static_cast<GLuint>(state.previewTexture);
				glDeleteTextures(1, &id);
				state.previewTexture = 0u;
			}
		}

		bool LoadSpriteSheetSourceImage(const std::string &inputPath, SpriteSheetSourceImage &outImage, std::string &outError)
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

		bool IsSupportedImagePath(const std::string &path)
		{
			const std::string ext = ToLowerCopy(std::filesystem::path(path).extension().string());
			return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga";
		}

		bool TryAddSourceToSpriteSheetGenerator(SpriteSheetGeneratorState &state,
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

		bool TryReadPathFromDropPayload(const ImGuiPayload *payload, std::string &outPath)
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

		bool ComposeSpriteSheet(const SpriteSheetGeneratorState &state, SpriteSheetComposite &outComposite, std::string &outError)
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

		void UploadSpriteSheetPreviewTexture(SpriteSheetGeneratorState &state)
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

		bool WriteTgaRgba(const std::string &path,
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

		void MarkSpriteSheetGeneratorDirty()
		{
			++SpriteSheetGenerator().dirtyRevision;
		}

		void RebuildSpriteSheetGeneratorPreview()
		{
			SpriteSheetComposite composite;
			std::string composeError;
			auto &state = SpriteSheetGenerator();
			if (!ComposeSpriteSheet(state, composite, composeError))
			{
				state.preview = {};
				DestroySpriteSheetPreviewTexture(state);
				state.status = composeError;
				state.builtRevision = state.dirtyRevision;
				return;
			}

			state.preview = std::move(composite);
			UploadSpriteSheetPreviewTexture(state);
			state.status = composeError;
			state.builtRevision = state.dirtyRevision;
		}
	}

	void DrawSpriteSheetGeneratorWindowImpl()
	{
		SpriteSheetGeneratorState &state = SpriteSheetGenerator();
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
}
