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
#pragma endregion

#pragma region Declarations
struct Material;

class ShaderManager;
class TextureManager;

class Renderer
{
public:
	static constexpr int MAX_LIGHTS = 10;

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

	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;
	glm::mat4 m_view{};
	glm::mat4 m_proj{};
	std::vector<LightData> m_lights;
	std::vector<GpuMesh> m_gpuMeshes;
	std::unordered_map<std::string, uint32_t> m_meshCacheByKey;
	#pragma endregion
};
#pragma endregion
