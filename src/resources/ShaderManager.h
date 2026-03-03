/**
 * @file ShaderManager.h
 * @brief Declarations for ShaderManager.
 */

#pragma once

#pragma region Includes
#include <unordered_map>
#include <string>
#include <memory>

#include "ResourceHandle.h"
#include "../shader/shader.h"
#pragma endregion

#pragma region Declarations
class ShaderManager
{
public:
	#pragma region Public Interface
	using ShaderHandle = ResourceHandle<Shader>;

	/**
	 * @brief Executes Load.
	 */
	ShaderHandle Load(const std::string &name,
					  const std::string &vsPath,
					  const std::string &fsPath);

	/**
	 * @brief Executes Load From Source.
	 */
	ShaderHandle LoadFromSource(const std::string &name,
								std::string vsSource,
								std::string fsSource,
								std::string vsPath = {},
								std::string fsPath = {});

	/**
	 * @brief Executes Get.
	 */
	Shader *Get(ShaderHandle handle);

	/**
	 * @brief Executes Reload All.
	 */
	void ReloadAll();

	#pragma endregion
private:
	#pragma region Private Implementation
	struct Entry
	{
		std::unique_ptr<Shader> shader;
		std::string vsPath;
		std::string fsPath;
		bool hasPaths = false;
	};

	uint32_t m_nextId = 1;

	std::unordered_map<uint32_t, Entry> m_entries;
	std::unordered_map<std::string, uint32_t> m_nameToId;
	#pragma endregion
};
#pragma endregion
