#pragma region Includes
#include "ShaderManager.h"
#include "../fileManager/fileManager.h"
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

    auto shader = std::make_unique<Shader>();

    shader->Init(
        FileManager::read(vsPath),
        FileManager::read(fsPath));

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

    auto shader = std::make_unique<Shader>();
    shader->Init(vsSource, fsSource);

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

        entry.shader->Init(
            FileManager::read(entry.vsPath),
            FileManager::read(entry.fsPath));
    }
}
#pragma endregion
