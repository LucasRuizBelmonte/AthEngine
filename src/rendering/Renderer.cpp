#pragma region Includes
#include <GL/glew.h>

#include "Renderer.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../shader/shader.h"
#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"
#include "../rendering/Texture.h"
#include <glm/gtc/matrix_inverse.hpp>
#pragma endregion

#pragma region Function Definitions
Renderer::Renderer(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager), m_textureManager(textureManager)
{
}

void Renderer::BeginFrame(int width, int height)
{
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetCamera(const glm::mat4 &view, const glm::mat4 &proj)
{
	m_view = view;
	m_proj = proj;
}

void Renderer::Submit(const Mesh &mesh,
					  const Material &material,
					  const glm::mat4 &model)
{
	Shader *shader = m_shaderManager.Get(material.shader);
	if (!shader)
		return;

	shader->Use();

	shader->SetUniform("u_model", model);
	shader->SetUniform("u_view", m_view);
	shader->SetUniform("u_proj", m_proj);
	shader->SetUniform("u_tint", material.tint);
	shader->SetUniform("u_specularStrength", material.specularStrength);
	shader->SetUniform("u_shininess", material.shininess);
	shader->SetUniform("u_normalStrength", material.normalStrength);
	shader->SetUniform("u_emissionStrength", material.emissionStrength);

	const glm::mat4 invView = glm::inverse(m_view);
	const glm::vec3 viewPos = glm::vec3(invView[3]);
	shader->SetUniform("u_viewPos", viewPos);
	shader->SetUniform("u_lightDir", glm::normalize(glm::vec3(-0.4f, -1.0f, -0.25f)));

	auto bindTextureSlot = [&](GLenum textureUnit,
	                           int samplerUnit,
	                           const ResourceHandle<Texture> &handle,
	                           const char *samplerUniform,
	                           const char *hasUniform)
	{
		bool bound = false;
		if (handle.IsValid())
		{
			Texture *tex = m_textureManager.Get(handle);
			if (tex && tex->GetId())
			{
				glActiveTexture(textureUnit);
				glBindTexture(GL_TEXTURE_2D, tex->GetId());
				shader->SetUniform(samplerUniform, samplerUnit);
				bound = true;
			}
		}
		shader->SetUniform(hasUniform, bound);
	};

	bindTextureSlot(GL_TEXTURE0, 0, material.texture, "u_texBaseColor", "u_hasBaseColorTex");
	shader->SetUniform("u_tex0", 0);
	bindTextureSlot(GL_TEXTURE1, 1, material.specularTexture, "u_texSpecular", "u_hasSpecularTex");
	bindTextureSlot(GL_TEXTURE2, 2, material.normalTexture, "u_texNormal", "u_hasNormalTex");
	bindTextureSlot(GL_TEXTURE3, 3, material.emissionTexture, "u_texEmission", "u_hasEmissionTex");

	glBindVertexArray(mesh.vao);
	glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
#pragma endregion
