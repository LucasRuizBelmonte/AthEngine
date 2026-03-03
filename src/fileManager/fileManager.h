/**
 * @file fileManager.h
 * @brief Declarations for fileManager.
 */

#pragma once

#pragma region Includes
#include <string>
#pragma endregion

#pragma region Declarations
class FileManager
{
public:
	#pragma region Public Interface
	/**
	 * @brief Constructs a new FileManager instance.
	 */
	FileManager();
	/**
	 * @brief Destroys this FileManager instance.
	 */
	~FileManager();

	/**
	 * @brief Executes read.
	 */
	static std::string read(const std::string& filePath);
	#pragma endregion
};
#pragma endregion
