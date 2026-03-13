#pragma region Includes
#include "ShaderManager.h"
#include "../fileManager/fileManager.h"

#include <cstdio>
#pragma endregion

#pragma region Function Definitions
ShaderManager::ShaderHandle
ShaderManager::Load(const std::string &name,
                    const std::string &vsPath,
                    const std::string &fsPath)
{
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end())
        return {it->second};

    if (vsPath.empty() || fsPath.empty())
    {
        std::fprintf(stderr, "[ShaderManager] Shader paths are empty for '%s' (vs='%s', fs='%s').\n",
                     name.c_str(),
                     vsPath.c_str(),
                     fsPath.c_str());
        return {0};
    }

    const std::string vsSource = FileManager::read(vsPath);
    const std::string fsSource = FileManager::read(fsPath);
    if (vsSource.empty() || fsSource.empty())
    {
        std::fprintf(stderr, "[ShaderManager] Failed to read shader source for '%s' (vs='%s', fs='%s').\n",
                     name.c_str(),
                     vsPath.c_str(),
                     fsPath.c_str());
        return {0};
    }

    auto shader = std::make_unique<Shader>();
    if (!shader->Init(vsSource, fsSource))
    {
        std::fprintf(stderr, "[ShaderManager] Failed to compile shader '%s' (vs='%s', fs='%s').\n",
                     name.c_str(),
                     vsPath.c_str(),
                     fsPath.c_str());
        return {0};
    }

    uint32_t id = m_nextId++;

    Entry entry;
    entry.shader = std::move(shader);
    entry.vsPath = vsPath;
    entry.fsPath = fsPath;
    entry.hasPaths = true;

    m_entries[id] = std::move(entry);
    m_nameToId[name] = id;

    return {id};
}

ShaderManager::ShaderHandle
ShaderManager::LoadFromSource(const std::string &name,
                              std::string vsSource,
                              std::string fsSource,
                              std::string vsPath,
                              std::string fsPath)
{
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end())
        return {it->second};

    if (vsSource.empty() || fsSource.empty())
    {
        std::fprintf(stderr, "[ShaderManager] Shader source is empty for '%s'.\n", name.c_str());
        return {0};
    }

    auto shader = std::make_unique<Shader>();
    if (!shader->Init(vsSource, fsSource))
    {
        std::fprintf(stderr, "[ShaderManager] Failed to compile in-memory shader '%s'.\n", name.c_str());
        return {0};
    }

    uint32_t id = m_nextId++;

    Entry entry;
    entry.shader = std::move(shader);
    entry.vsPath = std::move(vsPath);
    entry.fsPath = std::move(fsPath);
    entry.hasPaths = !entry.vsPath.empty() && !entry.fsPath.empty();

    m_entries[id] = std::move(entry);
    m_nameToId[name] = id;

    return {id};
}

Shader *ShaderManager::Get(ShaderHandle handle)
{
    auto it = m_entries.find(handle.id);
    if (it == m_entries.end())
        return nullptr;
    return it->second.shader.get();
}

std::string ShaderManager::GetFragmentPath(ShaderHandle handle) const
{
    auto it = m_entries.find(handle.id);
    if (it == m_entries.end())
        return {};
    return it->second.fsPath;
}

std::string ShaderManager::GetVertexPath(ShaderHandle handle) const
{
    auto it = m_entries.find(handle.id);
    if (it == m_entries.end())
        return {};
    return it->second.vsPath;
}

void ShaderManager::ReloadAll()
{
    for (auto &kv : m_entries)
    {
        auto &entry = kv.second;
        if (!entry.hasPaths)
            continue;

        const std::string vsSource = FileManager::read(entry.vsPath);
        const std::string fsSource = FileManager::read(entry.fsPath);
        if (vsSource.empty() || fsSource.empty())
        {
            std::fprintf(stderr, "[ShaderManager] Skipping shader reload due to missing source (vs='%s', fs='%s').\n",
                         entry.vsPath.c_str(),
                         entry.fsPath.c_str());
            continue;
        }

        if (!entry.shader->Init(vsSource, fsSource))
        {
            std::fprintf(stderr, "[ShaderManager] Shader reload failed (vs='%s', fs='%s').\n",
                         entry.vsPath.c_str(),
                         entry.fsPath.c_str());
        }
    }
}
#pragma endregion
