/**
 * @file SparseSet.h
 * @brief Declarations for SparseSet.
 */

#pragma once

#pragma region Includes
#include <vector>
#include <cstddef>
#include <utility>
#include "Entity.h"
#pragma endregion

#pragma region Declarations
template <typename T>
class SparseSet
{
public:
#pragma region Public Interface
	static constexpr size_t npos = static_cast<size_t>(-1);

	/**
	 * @brief Executes Has.
	 */
	bool Has(Entity e) const
	{
		return IndexOf(e) != npos;
	}

	/**
	 * @brief Executes Is Enabled.
	 */
	bool IsEnabled(Entity e) const
	{
		const size_t idx = IndexOf(e);
		if (idx == npos || idx >= m_denseEnabled.size())
			return false;
		return m_denseEnabled[idx] != 0u;
	}

	/**
	 * @brief Executes Set Enabled.
	 */
	void SetEnabled(Entity e, bool enabled)
	{
		const size_t idx = IndexOf(e);
		if (idx == npos || idx >= m_denseEnabled.size())
			return;
		m_denseEnabled[idx] = enabled ? 1u : 0u;
	}

	/**
	 * @brief Executes Get.
	 */
	T &Get(Entity e)
	{
		return m_denseComponents[m_sparse[EntityIdOf(e)]];
	}

	/**
	 * @brief Executes Get.
	 */
	const T &Get(Entity e) const
	{
		return m_denseComponents[m_sparse[EntityIdOf(e)]];
	}

	/**
	 * @brief Executes Emplace.
	 */
	template <typename... Args>
	T &Emplace(Entity e, Args &&...args)
	{
		EnsureSparse(e);
		if (Has(e))
		{
			const size_t idx = m_sparse[EntityIdOf(e)];
			m_denseComponents[idx] = T(std::forward<Args>(args)...);
			if (idx >= m_denseEnabled.size())
				m_denseEnabled.resize(m_denseComponents.size(), 1u);
			m_denseEnabled[idx] = 1u;
			return m_denseComponents[idx];
		}

		size_t idx = m_denseEntities.size();
		m_denseEntities.push_back(e);
		m_denseComponents.emplace_back(std::forward<Args>(args)...);
		m_denseEnabled.push_back(1u);
		m_sparse[EntityIdOf(e)] = idx;
		return m_denseComponents.back();
	}

	/**
	 * @brief Executes Remove.
	 */
	void Remove(Entity e)
	{
		if (!Has(e))
			return;

		const uint32_t id = EntityIdOf(e);
		size_t idx = m_sparse[id];
		size_t last = m_denseEntities.size() - 1;

		if (idx != last)
		{
			Entity movedEntity = m_denseEntities[last];
			m_denseEntities[idx] = movedEntity;
			m_denseComponents[idx] = std::move(m_denseComponents[last]);
			m_denseEnabled[idx] = m_denseEnabled[last];
			m_sparse[EntityIdOf(movedEntity)] = idx;
		}

		m_denseEntities.pop_back();
		m_denseComponents.pop_back();
		m_denseEnabled.pop_back();
		m_sparse[id] = npos;
	}

	/**
	 * @brief Executes Entities.
	 */
	const std::vector<Entity> &Entities() const { return m_denseEntities; }
	/**
	 * @brief Executes Entities.
	 */
	std::vector<Entity> &Entities() { return m_denseEntities; }

	/**
	 * @brief Executes Size.
	 */
	size_t Size() const { return m_denseEntities.size(); }

#pragma endregion
private:
#pragma region Private Implementation
	/**
	 * @brief Executes Index Of.
	 */
	size_t IndexOf(Entity e) const
	{
		const uint32_t id = EntityIdOf(e);
		if (id >= m_sparse.size())
			return npos;
		const size_t idx = m_sparse[id];
		if (idx == npos || idx >= m_denseEntities.size())
			return npos;
		return (m_denseEntities[idx] == e) ? idx : npos;
	}

	/**
	 * @brief Executes Ensure Sparse.
	 */
	void EnsureSparse(Entity e)
	{
		const uint32_t id = EntityIdOf(e);
		if (id < m_sparse.size())
			return;
		size_t newSize = static_cast<size_t>(id) + 1;
		m_sparse.resize(newSize, npos);
	}

	std::vector<size_t> m_sparse;
	std::vector<Entity> m_denseEntities;
	std::vector<T> m_denseComponents;
	std::vector<uint8_t> m_denseEnabled;
#pragma endregion
};
#pragma endregion
