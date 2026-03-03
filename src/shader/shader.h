/**
 * @file shader.h
 * @brief Declarations for shader.
 */

#pragma once

#pragma region Includes
#include <string>
#include <unordered_map>

#include <GL/glew.h>
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
class Shader
{
public:
	#pragma region Public Interface
	/**
	 * @brief Constructs a new Shader instance.
	 */
	Shader() = default;
	/**
	 * @brief Destroys this Shader instance.
	 */
	~Shader();

	/**
	 * @brief Constructs a new Shader instance.
	 */
	Shader(const Shader &) = delete;
	/**
	 * @brief Overloads operator=.
	 */
	Shader &operator=(const Shader &) = delete;

	/**
	 * @brief Constructs a new Shader instance.
	 */
	Shader(Shader &&other) noexcept;
	/**
	 * @brief Overloads operator=.
	 */
	Shader &operator=(Shader &&other) noexcept;

	/**
	 * @brief Executes Init.
	 */
	bool Init(const std::string &vertexCode, const std::string &fragmentCode);
	/**
	 * @brief Executes Use.
	 */
	void Use() const;

	/**
	 * @brief Executes Destroy.
	 */
	void Destroy();

	/**
	 * @brief Executes Get Program Id.
	 */
	GLuint GetProgramId() const { return m_programId; }

	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, bool value);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, int value);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, float value);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, const glm::vec2 &value);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, float x, float y);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, const glm::vec3 &value);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, float x, float y, float z);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, const glm::vec4 &value);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, float x, float y, float z, float w);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, const glm::mat2 &mat);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, const glm::mat3 &mat);
	/**
	 * @brief Executes Set Uniform.
	 */
	void SetUniform(const std::string &name, const glm::mat4 &mat);

	#pragma endregion
private:
	#pragma region Private Implementation
	/**
	 * @brief Executes Get Uniform Location Cached.
	 */
	GLint GetUniformLocationCached(const std::string &name);

	/**
	 * @brief Executes Compile And Link.
	 */
	bool CompileAndLink(const std::string &vertexCode, const std::string &fragmentCode);

	/**
	 * @brief Executes Check Compile.
	 */
	bool CheckCompile(GLuint shader, const char *label);
	/**
	 * @brief Executes Check Link.
	 */
	bool CheckLink(GLuint program);

	GLuint m_programId = 0;
	std::unordered_map<std::string, GLint> m_uniformCache;
	#pragma endregion
};
#pragma endregion
