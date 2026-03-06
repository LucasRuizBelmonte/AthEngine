#pragma region Includes
#include "Renderer.h"

#include "../platform/GL.h"
#include "../components/Material.h"
#include "../shader/shader.h"
#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"
#include "../rendering/Texture.h"
#include "../rendering/ModelLoader.h"
#include "../material/MaterialMetadata.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
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

	static void BuildQuadMeshData(std::vector<float> &outVertices, std::vector<uint32_t> &outIndices)
	{
		outVertices = {
			-0.5f, -0.5f, 0.0f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f,
			0.5f, -0.5f, 0.0f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f,
			0.5f, 0.5f, 0.0f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f,
			-0.5f, 0.5f, 0.0f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f};

		outIndices = {0, 1, 2, 2, 3, 0};
	}

	static std::string DeriveTexturePresenceUniform(const std::string &samplerUniform)
	{
		constexpr const char *kTexturePrefix = "u_tex";
		if (samplerUniform.rfind(kTexturePrefix, 0) != 0)
			return {};
		const std::string suffix = samplerUniform.substr(std::strlen(kTexturePrefix));
		if (suffix.empty())
			return {};
		return "u_has" + suffix + "Tex";
	}
}
#pragma endregion

#pragma region Function Definitions
Renderer::Renderer(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager), m_textureManager(textureManager)
{
	m_gpuMeshes.reserve(64);
}

Renderer::~Renderer()
{
	DestroyUiBatchBuffers();
	DestroyAllGpuMeshes();
}

void Renderer::BeginFrame(int width, int height)
{
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::ResetFrameStats()
{
	m_frameStats = {};
}

const Renderer::FrameStats &Renderer::GetFrameStats() const
{
	return m_frameStats;
}

void Renderer::SetCamera(const glm::mat4 &view, const glm::mat4 &proj)
{
	m_view = view;
	m_proj = proj;
}

void Renderer::SetLights(const std::vector<LightData> &lights)
{
	const size_t count = std::min(lights.size(), static_cast<size_t>(MAX_LIGHTS));
	m_lights.assign(lights.begin(), lights.begin() + static_cast<std::ptrdiff_t>(count));
}

uint32_t Renderer::UploadMesh(const std::vector<float> &vertices,
                              const std::vector<uint32_t> &indices,
                              uint32_t vertexStrideBytes)
{
	if (vertices.empty() || indices.empty() || vertexStrideBytes == 0u)
		return 0;

	GpuMesh gpu{};
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	gpu.vao = static_cast<uint32_t>(vao);
	gpu.vbo = static_cast<uint32_t>(vbo);
	gpu.ebo = static_cast<uint32_t>(ebo);

	if (gpu.vao == 0 || gpu.vbo == 0 || gpu.ebo == 0)
	{
		DestroyGpuMesh(gpu);
		return 0;
	}

	glBindVertexArray(static_cast<GLuint>(gpu.vao));

	glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(gpu.vbo));
	glBufferData(GL_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
	             vertices.data(),
	             GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(gpu.ebo));
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
	             indices.data(),
	             GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(vertexStrideBytes), reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(vertexStrideBytes), reinterpret_cast<void *>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(vertexStrideBytes), reinterpret_cast<void *>(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(vertexStrideBytes), reinterpret_cast<void *>(8 * sizeof(float)));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	gpu.indexCount = static_cast<uint32_t>(indices.size());
	m_gpuMeshes.push_back(gpu);
	return static_cast<uint32_t>(m_gpuMeshes.size());
}

uint32_t Renderer::AcquireOrCreateMesh(const std::string &cacheKey, const std::string &resolvedPath)
{
	if (cacheKey.empty() || resolvedPath.empty())
		return 0;

	auto it = m_meshCacheByKey.find(cacheKey);
	if (it != m_meshCacheByKey.end())
		return it->second;

	const ModelLoader::MeshData loaded = ModelLoader::LoadFirstMeshData(resolvedPath);
	std::vector<float> vertices;
	vertices.reserve(loaded.vertices.size() * 11u);
	for (const ModelLoader::VertexPUNT &v : loaded.vertices)
	{
		vertices.push_back(v.px);
		vertices.push_back(v.py);
		vertices.push_back(v.pz);
		vertices.push_back(v.u);
		vertices.push_back(v.v);
		vertices.push_back(v.nx);
		vertices.push_back(v.ny);
		vertices.push_back(v.nz);
		vertices.push_back(v.tx);
		vertices.push_back(v.ty);
		vertices.push_back(v.tz);
	}

	const uint32_t meshId = UploadMesh(vertices, loaded.indices, 11u * static_cast<uint32_t>(sizeof(float)));
	if (meshId == 0)
		return 0;

	m_meshCacheByKey[cacheKey] = meshId;
	return meshId;
}

uint32_t Renderer::AcquireMesh(const std::string &meshPath)
{
	if (meshPath.empty())
		return 0;
	if (meshPath == "builtin://quad")
		return AcquireBuiltinQuad();

	const std::string resolvedPath = ResolveRuntimeAssetPath(meshPath);
	if (resolvedPath.empty())
		return 0;

	try
	{
		return AcquireOrCreateMesh(resolvedPath, resolvedPath);
	}
	catch (...)
	{
		return 0;
	}
}

uint32_t Renderer::AcquireBuiltinQuad()
{
	constexpr const char *kQuadKey = "builtin://quad";
	auto it = m_meshCacheByKey.find(kQuadKey);
	if (it != m_meshCacheByKey.end())
		return it->second;

	std::vector<float> vertices;
	std::vector<uint32_t> indices;
	BuildQuadMeshData(vertices, indices);

	const uint32_t meshId = UploadMesh(vertices, indices, 11u * static_cast<uint32_t>(sizeof(float)));
	if (meshId == 0)
		return 0;

	m_meshCacheByKey[kQuadKey] = meshId;
	return meshId;
}

uint32_t Renderer::GetMeshIndexCount(uint32_t meshId) const
{
	if (meshId == 0 || meshId > m_gpuMeshes.size())
		return 0;
	return m_gpuMeshes[meshId - 1].indexCount;
}

void Renderer::SubmitMesh(uint32_t meshId,
                          const Material &material,
                          const glm::mat4 &model)
{
	if (meshId == 0 || meshId > m_gpuMeshes.size())
		return;

	const GpuMesh &mesh = m_gpuMeshes[meshId - 1];
	if (mesh.vao == 0 || mesh.indexCount == 0)
		return;

	Shader *shader = m_shaderManager.Get(material.shader);
	if (!shader)
		return;

	shader->Use();

	shader->SetUniform("u_model", model);
	shader->SetUniform("u_view", m_view);
	shader->SetUniform("u_proj", m_proj);

	const glm::mat4 invView = glm::inverse(m_view);
	const glm::vec3 viewPos = glm::vec3(invView[3]);
	shader->SetUniform("u_viewPos", viewPos);
	if (!m_lights.empty())
		shader->SetUniform("u_lightDir", m_lights[0].direction);
	else
		shader->SetUniform("u_lightDir", glm::normalize(glm::vec3(-0.4f, -1.0f, -0.25f)));

	shader->SetUniform("lightCount", static_cast<int>(m_lights.size()));
	for (size_t i = 0; i < m_lights.size(); ++i)
	{
		const LightData &l = m_lights[i];
		const std::string prefix = "lights[" + std::to_string(i) + "]";
		shader->SetUniform(prefix + ".type", l.type);
		shader->SetUniform(prefix + ".position", l.position);
		shader->SetUniform(prefix + ".direction", l.direction);
		shader->SetUniform(prefix + ".color", l.color);
		shader->SetUniform(prefix + ".intensity", l.intensity);
		shader->SetUniform(prefix + ".range", l.range);
		shader->SetUniform(prefix + ".innerCone", l.innerCone);
		shader->SetUniform(prefix + ".outerCone", l.outerCone);
	}

	std::string shaderPath = material.shaderPath;
	if (shaderPath.empty())
		shaderPath = m_shaderManager.GetFragmentPath(material.shader);
	const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(shaderPath);
	if (metadata.Empty())
		return;

	auto bindTextureSlot = [&](GLenum textureUnit,
	                           int samplerUnit,
	                           const ResourceHandle<Texture> &handle,
	                           const std::string &samplerUniform,
	                           const std::string &hasUniform)
	{
		bool bound = false;
		if (handle.IsValid())
		{
			Texture *tex = m_textureManager.Get(handle);
			if (tex && tex->GetId())
			{
				glActiveTexture(textureUnit);
				glBindTexture(GL_TEXTURE_2D, tex->GetId());
				bound = true;
			}
		}
		shader->SetUniform(samplerUniform, samplerUnit);
		if (!hasUniform.empty())
			shader->SetUniform(hasUniform, bound);
		return bound;
	};

	std::vector<GLenum> usedTextureUnits;
	usedTextureUnits.reserve(8);

	int samplerUnit = 0;
	for (const MaterialParameterMetadata &desc : metadata.parameters)
	{
		const auto it = material.parameters.find(desc.name);

		switch (desc.type)
		{
		case MaterialParameterType::Float:
		{
			const float value = (it != material.parameters.end()) ? it->second.numericValue.x : desc.defaultNumeric.x;
			shader->SetUniform(desc.name, value);
			break;
		}
		case MaterialParameterType::Vec2:
		{
			const glm::vec2 value = (it != material.parameters.end())
			                            ? glm::vec2(it->second.numericValue)
			                            : glm::vec2(desc.defaultNumeric);
			shader->SetUniform(desc.name, value);
			break;
		}
		case MaterialParameterType::Vec3:
		{
			const glm::vec3 value = (it != material.parameters.end())
			                            ? glm::vec3(it->second.numericValue)
			                            : glm::vec3(desc.defaultNumeric);
			shader->SetUniform(desc.name, value);
			break;
		}
		case MaterialParameterType::Vec4:
		{
			const glm::vec4 value = (it != material.parameters.end())
			                            ? it->second.numericValue
			                            : desc.defaultNumeric;
			shader->SetUniform(desc.name, value);
			break;
		}
		case MaterialParameterType::Texture2D:
		{
			const GLenum textureUnit = static_cast<GLenum>(GL_TEXTURE0 + samplerUnit);
			const std::string hasUniform = DeriveTexturePresenceUniform(desc.name);
			const bool wasBound = bindTextureSlot(textureUnit,
			                                      samplerUnit,
			                                      (it != material.parameters.end()) ? it->second.texture : ResourceHandle<Texture>{0},
			                                      desc.name,
			                                      hasUniform);
			if (wasBound)
				usedTextureUnits.push_back(textureUnit);
			++samplerUnit;
			break;
		}
		default:
			break;
		}
	}

	glBindVertexArray(static_cast<GLuint>(mesh.vao));
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	++m_frameStats.drawCalls;
	m_frameStats.indices += static_cast<uint64_t>(mesh.indexCount);
	m_frameStats.triangles += static_cast<uint64_t>(mesh.indexCount / 3u);

	for (auto it = usedTextureUnits.rbegin(); it != usedTextureUnits.rend(); ++it)
	{
		glActiveTexture(*it);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glActiveTexture(GL_TEXTURE0);
}

void Renderer::SubmitUnlitQuad(uint32_t meshId,
                               const ResourceHandle<Shader> &shaderHandle,
                               const ResourceHandle<Texture> &textureHandle,
                               const glm::mat4 &model,
                               const glm::vec4 &tint,
                               const glm::vec4 &uvRect)
{
	if (meshId == 0 || meshId > m_gpuMeshes.size())
		return;

	const GpuMesh &mesh = m_gpuMeshes[meshId - 1];
	if (mesh.vao == 0 || mesh.indexCount == 0)
		return;

	Shader *shader = m_shaderManager.Get(shaderHandle);
	if (!shader)
		return;

	shader->Use();
	shader->SetUniform("u_model", model);
	shader->SetUniform("u_view", m_view);
	shader->SetUniform("u_proj", m_proj);
	shader->SetUniform("u_tint", tint);
	shader->SetUniform("u_uvRect", uvRect);

	bool bound = false;
	if (textureHandle.IsValid())
	{
		Texture *texture = m_textureManager.Get(textureHandle);
		if (texture && texture->GetId() != 0u)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->GetId());
			bound = true;
		}
	}

	shader->SetUniform("u_texBaseColor", 0);
	shader->SetUniform("u_hasBaseColorTex", bound);

	glBindVertexArray(static_cast<GLuint>(mesh.vao));
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	++m_frameStats.drawCalls;
	m_frameStats.indices += static_cast<uint64_t>(mesh.indexCount);
	m_frameStats.triangles += static_cast<uint64_t>(mesh.indexCount / 3u);

	if (bound)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void Renderer::SubmitUnlitQuadBatch(const ResourceHandle<Shader> &shaderHandle,
                                    const ResourceHandle<Texture> &textureHandle,
                                    const std::vector<UnlitBatchVertex> &vertices,
                                    const std::vector<uint32_t> &indices,
                                    const glm::mat4 &view,
                                    const glm::mat4 &proj)
{
	if (vertices.empty() || indices.empty())
		return;

	EnsureUiBatchBuffers();
	if (m_uiBatchVao == 0u || m_uiBatchVbo == 0u || m_uiBatchEbo == 0u)
		return;

	Shader *shader = m_shaderManager.Get(shaderHandle);
	if (!shader)
		return;

	shader->Use();
	shader->SetUniform("u_view", view);
	shader->SetUniform("u_proj", proj);

	bool bound = false;
	if (textureHandle.IsValid())
	{
		Texture *texture = m_textureManager.Get(textureHandle);
		if (texture && texture->GetId() != 0u)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->GetId());
			bound = true;
		}
	}

	shader->SetUniform("u_texBaseColor", 0);
	shader->SetUniform("u_hasBaseColorTex", bound);

	glBindVertexArray(static_cast<GLuint>(m_uiBatchVao));
	glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_uiBatchVbo));
	glBufferData(GL_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(vertices.size() * sizeof(UnlitBatchVertex)),
	             vertices.data(),
	             GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(m_uiBatchEbo));
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
	             indices.data(),
	             GL_DYNAMIC_DRAW);

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	++m_frameStats.drawCalls;
	m_frameStats.indices += static_cast<uint64_t>(indices.size());
	m_frameStats.triangles += static_cast<uint64_t>(indices.size() / 3u);

	if (bound)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void Renderer::EnsureUiBatchBuffers()
{
	if (m_uiBatchVao != 0u && m_uiBatchVbo != 0u && m_uiBatchEbo != 0u)
		return;

	GLuint vao = 0u;
	GLuint vbo = 0u;
	GLuint ebo = 0u;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	if (vao == 0u || vbo == 0u || ebo == 0u)
	{
		if (vao != 0u)
			glDeleteVertexArrays(1, &vao);
		if (vbo != 0u)
			glDeleteBuffers(1, &vbo);
		if (ebo != 0u)
			glDeleteBuffers(1, &ebo);
		return;
	}

	m_uiBatchVao = static_cast<uint32_t>(vao);
	m_uiBatchVbo = static_cast<uint32_t>(vbo);
	m_uiBatchEbo = static_cast<uint32_t>(ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glVertexAttribPointer(0,
	                      2,
	                      GL_FLOAT,
	                      GL_FALSE,
	                      static_cast<GLsizei>(sizeof(UnlitBatchVertex)),
	                      reinterpret_cast<void *>(offsetof(UnlitBatchVertex, position)));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1,
	                      2,
	                      GL_FLOAT,
	                      GL_FALSE,
	                      static_cast<GLsizei>(sizeof(UnlitBatchVertex)),
	                      reinterpret_cast<void *>(offsetof(UnlitBatchVertex, uv)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2,
	                      4,
	                      GL_FLOAT,
	                      GL_FALSE,
	                      static_cast<GLsizei>(sizeof(UnlitBatchVertex)),
	                      reinterpret_cast<void *>(offsetof(UnlitBatchVertex, color)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void Renderer::DestroyUiBatchBuffers()
{
	if (m_uiBatchVao != 0u)
	{
		const GLuint vao = static_cast<GLuint>(m_uiBatchVao);
		glDeleteVertexArrays(1, &vao);
		m_uiBatchVao = 0u;
	}

	if (m_uiBatchVbo != 0u)
	{
		const GLuint vbo = static_cast<GLuint>(m_uiBatchVbo);
		glDeleteBuffers(1, &vbo);
		m_uiBatchVbo = 0u;
	}

	if (m_uiBatchEbo != 0u)
	{
		const GLuint ebo = static_cast<GLuint>(m_uiBatchEbo);
		glDeleteBuffers(1, &ebo);
		m_uiBatchEbo = 0u;
	}
}

void Renderer::DestroyGpuMesh(GpuMesh &mesh)
{
	if (mesh.vao != 0)
	{
		const GLuint vao = static_cast<GLuint>(mesh.vao);
		glDeleteVertexArrays(1, &vao);
	}
	if (mesh.vbo != 0)
	{
		const GLuint vbo = static_cast<GLuint>(mesh.vbo);
		glDeleteBuffers(1, &vbo);
	}
	if (mesh.ebo != 0)
	{
		const GLuint ebo = static_cast<GLuint>(mesh.ebo);
		glDeleteBuffers(1, &ebo);
	}
	mesh = {};
}

void Renderer::DestroyAllGpuMeshes()
{
	for (GpuMesh &mesh : m_gpuMeshes)
		DestroyGpuMesh(mesh);
	m_gpuMeshes.clear();
	m_meshCacheByKey.clear();
}
#pragma endregion
