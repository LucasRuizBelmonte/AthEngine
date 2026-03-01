#include <GL/glew.h>

#include "Renderer.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../shader/shader.h"
#include "../resources/ShaderManager.h"

Renderer::Renderer(ShaderManager &shaderManager)
	: m_shaderManager(shaderManager)
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

	shader->use();
	shader->setUniform("u_model", model);
	shader->setUniform("u_view", m_view);
	shader->setUniform("u_proj", m_proj);

	glBindVertexArray(mesh.vao);
	glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}