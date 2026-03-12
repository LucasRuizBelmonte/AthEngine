/**
 * @file Renderer.h
 * @brief Declarations for Renderer.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../resources/ResourceHandle.h"
#pragma endregion

#pragma region Declarations
struct Material;
class Shader;
class Texture;

class ShaderManager;
class TextureManager;

class Renderer
{
public:
	static constexpr int MAX_LIGHTS = 10;

	struct FrameStats
	{
		uint32_t drawCalls = 0u;
		uint64_t triangles = 0u;
		uint64_t indices = 0u;
	};

	struct UnlitBatchVertex
	{
		glm::vec2 position{0.0f, 0.0f};
		glm::vec2 uv{0.0f, 0.0f};
		glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
	};

	struct LightData
	{
		int type = 0;
		glm::vec3 position{0.0f, 0.0f, 0.0f};
		glm::vec3 direction{0.0f, -1.0f, 0.0f};
		glm::vec3 color{1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;
		float range = 10.0f;
		float innerCone = 0.5f;
		float outerCone = 0.7f;
	};

	#pragma region Public Interface
	/**
	 * @brief Constructs a new Renderer instance.
	 */
	Renderer(ShaderManager &shaderManager, TextureManager &textureManager);
	~Renderer();

	/**
	 * @brief Executes Begin Frame.
	 */
	void BeginFrame(int width, int height);
	/**
	 * @brief Resets per-frame render counters.
	 */
	void ResetFrameStats();
	/**
	 * @brief Returns per-frame render counters.
	 */
	const FrameStats &GetFrameStats() const;
	/**
	 * @brief Executes Set Camera.
	 */
	void SetCamera(const glm::mat4 &view, const glm::mat4 &proj);
	/**
	 * @brief Executes Set Lights.
	 */
	void SetLights(const std::vector<LightData> &lights);

	/**
	 * @brief Returns a renderer-owned GPU mesh id for an asset path.
	 */
	uint32_t AcquireMesh(const std::string &meshPath);

	/**
	 * @brief Returns a renderer-owned GPU mesh id for the built-in quad.
	 */
	uint32_t AcquireBuiltinQuad();

	/**
	 * @brief Returns index count for a GPU mesh id.
	 */
	uint32_t GetMeshIndexCount(uint32_t meshId) const;

	/**
	 * @brief Executes Submit.
	 */
	void SubmitMesh(uint32_t meshId,
	                const Material &material,
	                const glm::mat4 &model);

	/**
	 * @brief Executes a lightweight unlit quad submission path used by runtime UI.
	 */
	void SubmitUnlitQuad(uint32_t meshId,
	                     const ResourceHandle<Shader> &shaderHandle,
	                     const ResourceHandle<Texture> &textureHandle,
	                     const glm::mat4 &model,
	                     const glm::vec4 &tint,
	                     const glm::vec4 &uvRect);

	/**
	 * @brief Executes a dynamic batched unlit draw used by UI.
	 */
	void SubmitUnlitQuadBatch(const ResourceHandle<Shader> &shaderHandle,
	                          const ResourceHandle<Texture> &textureHandle,
	                          const std::vector<UnlitBatchVertex> &vertices,
	                          const std::vector<uint32_t> &indices,
	                          const glm::mat4 &view,
	                          const glm::mat4 &proj);

	#pragma endregion
private:
	#pragma region Private Implementation
	struct GpuMesh
	{
		uint32_t vao = 0;
		uint32_t vbo = 0;
		uint32_t ebo = 0;
		uint32_t indexCount = 0;
	};

	uint32_t UploadMesh(const std::vector<float> &vertices,
	                   const std::vector<uint32_t> &indices,
	                   uint32_t vertexStrideBytes);
	uint32_t AcquireOrCreateMesh(const std::string &cacheKey, const std::string &resolvedPath);
	void DestroyGpuMesh(GpuMesh &mesh);
	void DestroyAllGpuMeshes();
	void EnsureUiBatchBuffers();
	void DestroyUiBatchBuffers();

	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;
	glm::mat4 m_view{};
	glm::mat4 m_proj{};
	glm::vec3 m_viewPosition{0.0f, 0.0f, 0.0f};
	std::vector<LightData> m_lights;
	std::vector<GpuMesh> m_gpuMeshes;
	std::unordered_map<std::string, uint32_t> m_meshCacheByKey;
	FrameStats m_frameStats;
	uint32_t m_uiBatchVao = 0u;
	uint32_t m_uiBatchVbo = 0u;
	uint32_t m_uiBatchEbo = 0u;
	#pragma endregion
};
#pragma endregion
