#pragma once
#include <unordered_map>
#include <string>
#include <memory>

#include "ResourceHandle.h"
#include "../shader/shader.h"

class ShaderManager
{
public:
	using ShaderHandle = ResourceHandle<Shader>;

	ShaderHandle Load(const std::string &name,
					  const std::string &vsPath,
					  const std::string &fsPath);

	ShaderHandle LoadFromSource(const std::string &name,
								std::string vsSource,
								std::string fsSource,
								std::string vsPath = {},
								std::string fsPath = {});

	Shader *Get(ShaderHandle handle);

	void ReloadAll();

private:
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
};