#include "ShaderManager.h"
#include "../fileManager/fileManager.h"

ShaderManager::ShaderHandle
ShaderManager::Load(const std::string &name,
                    const std::string &vsPath,
                    const std::string &fsPath)
{
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end())
    {
        return {it->second};
    }

    auto shader = std::make_unique<Shader>();

    shader->Init(
        FileManager::read(vsPath),
        FileManager::read(fsPath));

    uint32_t id = m_nextId++;

    Entry entry;
    entry.shader = std::move(shader);
    entry.vsPath = vsPath;
    entry.fsPath = fsPath;

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

void ShaderManager::ReloadAll()
{
    for (auto &kv : m_entries)
    {
        auto &entry = kv.second;

        entry.shader->Init(
            FileManager::read(entry.vsPath),
            FileManager::read(entry.fsPath));
    }
}