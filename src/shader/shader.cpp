#pragma region Includes
#include "shader.h"

#include <iostream>
#include <utility>
#include <vector>
#pragma endregion

#pragma region Function Definitions
Shader::~Shader()
{
	Destroy();
}

Shader::Shader(Shader &&other) noexcept
{
	m_programId = other.m_programId;
	m_uniformCache = std::move(other.m_uniformCache);

	other.m_programId = 0;
	other.m_uniformCache.clear();
}

Shader &Shader::operator=(Shader &&other) noexcept
{
	if (this == &other)
		return *this;

	Destroy();

	m_programId = other.m_programId;
	m_uniformCache = std::move(other.m_uniformCache);

	other.m_programId = 0;
	other.m_uniformCache.clear();

	return *this;
}

void Shader::Destroy()
{
	if (m_programId)
		glDeleteProgram(m_programId);

	m_programId = 0;
	m_uniformCache.clear();
}

bool Shader::Init(const std::string &vertexCode, const std::string &fragmentCode)
{
	Destroy();
	return CompileAndLink(vertexCode, fragmentCode);
}

void Shader::Use() const
{
	glUseProgram(m_programId);
}

GLint Shader::GetUniformLocationCached(const std::string &name)
{
	auto it = m_uniformCache.find(name);
	if (it != m_uniformCache.end())
		return it->second;

	GLint loc = glGetUniformLocation(m_programId, name.c_str());
	m_uniformCache.emplace(name, loc);
	return loc;
}

void Shader::SetUniform(const std::string &name, bool value)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform1i(loc, (int)value);
}

void Shader::SetUniform(const std::string &name, int value)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform1i(loc, value);
}

void Shader::SetUniform(const std::string &name, float value)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform1f(loc, value);
}

void Shader::SetUniform(const std::string &name, const glm::vec2 &value)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform2fv(loc, 1, &value[0]);
}

void Shader::SetUniform(const std::string &name, float x, float y)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform2f(loc, x, y);
}

void Shader::SetUniform(const std::string &name, const glm::vec3 &value)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform3fv(loc, 1, &value[0]);
}

void Shader::SetUniform(const std::string &name, float x, float y, float z)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform3f(loc, x, y, z);
}

void Shader::SetUniform(const std::string &name, const glm::vec4 &value)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform4fv(loc, 1, &value[0]);
}

void Shader::SetUniform(const std::string &name, float x, float y, float z, float w)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniform4f(loc, x, y, z, w);
}

void Shader::SetUniform(const std::string &name, const glm::mat2 &mat)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniformMatrix2fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetUniform(const std::string &name, const glm::mat3 &mat)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniformMatrix3fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetUniform(const std::string &name, const glm::mat4 &mat)
{
	GLint loc = GetUniformLocationCached(name);
	if (loc >= 0)
		glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

bool Shader::CheckCompile(GLuint shader, const char *label)
{
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success)
		return true;

	GLint len = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

	std::vector<char> log((size_t)len + 1);
	glGetShaderInfoLog(shader, len, nullptr, log.data());

	std::cout << "Shader: compile error (" << label << ")\n"
			  << log.data() << std::endl;
	return false;
}

bool Shader::CheckLink(GLuint program)
{
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success)
		return true;

	GLint len = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

	std::vector<char> log((size_t)len + 1);
	glGetProgramInfoLog(program, len, nullptr, log.data());

	std::cout << "Shader: link error\n"
			  << log.data() << std::endl;
	return false;
}

bool Shader::CompileAndLink(const std::string &vertexCode, const std::string &fragmentCode)
{
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const char *vsrc = vertexCode.c_str();
	glShaderSource(vs, 1, &vsrc, nullptr);
	glCompileShader(vs);
	if (!CheckCompile(vs, "VERTEX"))
	{
		glDeleteShader(vs);
		return false;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fsrc = fragmentCode.c_str();
	glShaderSource(fs, 1, &fsrc, nullptr);
	glCompileShader(fs);
	if (!CheckCompile(fs, "FRAGMENT"))
	{
		glDeleteShader(vs);
		glDeleteShader(fs);
		return false;
	}

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

	glDeleteShader(vs);
	glDeleteShader(fs);

	if (!CheckLink(prog))
	{
		glDeleteProgram(prog);
		return false;
	}

	m_programId = prog;
	m_uniformCache.clear();
	return true;
}
#pragma endregion
