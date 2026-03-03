/**
 * @file Texture.h
 * @brief Declarations for Texture.
 */

#pragma once

#pragma region Includes
#include <GL/glew.h>
#include <cstdint>
#pragma endregion

#pragma region Declarations
class Texture
{
public:
	#pragma region Public Interface
	/**
	 * @brief Constructs a new Texture instance.
	 */
	Texture() = default;

	/**
	 * @brief Constructs a new Texture instance.
	 */
	Texture(const Texture &) = delete;
	/**
	 * @brief Overloads operator=.
	 */
	Texture &operator=(const Texture &) = delete;

	/**
	 * @brief Constructs a new Texture instance.
	 */
	Texture(Texture &&other) noexcept;
	/**
	 * @brief Overloads operator=.
	 */
	Texture &operator=(Texture &&other) noexcept;

	/**
	 * @brief Destroys this Texture instance.
	 */
	~Texture();

	/**
	 * @brief Executes Load From File.
	 */
	bool LoadFromFile(const char *path, bool flipY);
	/**
	 * @brief Executes Load From RGBA.
	 */
	bool LoadFromRGBA(int width, int height, const uint8_t *rgba);

	/**
	 * @brief Executes Destroy.
	 */
	void Destroy();

	/**
	 * @brief Executes Get Id.
	 */
	GLuint GetId() const { return m_id; }
	/**
	 * @brief Executes Get Width.
	 */
	int GetWidth() const { return m_width; }
	/**
	 * @brief Executes Get Height.
	 */
	int GetHeight() const { return m_height; }

	#pragma endregion
private:
	#pragma region Private Implementation
	GLuint m_id = 0;
	int m_width = 0;
	int m_height = 0;
	#pragma endregion
};
#pragma endregion
