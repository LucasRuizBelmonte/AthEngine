#pragma once
#include <vector>
#include <cstddef>
#include <utility>
#include "Entity.h"

template <typename T>
class SparseSet
{
public:
	static constexpr size_t npos = static_cast<size_t>(-1);

	bool Has(Entity e) const
	{
		if (e >= m_sparse.size())
			return false;
		size_t idx = m_sparse[e];
		return idx != npos && idx < m_denseEntities.size() && m_denseEntities[idx] == e;
	}

	T &Get(Entity e)
	{
		return m_denseComponents[m_sparse[e]];
	}

	const T &Get(Entity e) const
	{
		return m_denseComponents[m_sparse[e]];
	}

	template <typename... Args>
	T &Emplace(Entity e, Args &&...args)
	{
		EnsureSparse(e);
		if (Has(e))
		{
			m_denseComponents[m_sparse[e]] = T(std::forward<Args>(args)...);
			return m_denseComponents[m_sparse[e]];
		}

		size_t idx = m_denseEntities.size();
		m_denseEntities.push_back(e);
		m_denseComponents.emplace_back(std::forward<Args>(args)...);
		m_sparse[e] = idx;
		return m_denseComponents.back();
	}

	void Remove(Entity e)
	{
		if (!Has(e))
			return;

		size_t idx = m_sparse[e];
		size_t last = m_denseEntities.size() - 1;

		if (idx != last)
		{
			Entity movedEntity = m_denseEntities[last];
			m_denseEntities[idx] = movedEntity;
			m_denseComponents[idx] = std::move(m_denseComponents[last]);
			m_sparse[movedEntity] = idx;
		}

		m_denseEntities.pop_back();
		m_denseComponents.pop_back();
		m_sparse[e] = npos;
	}

	const std::vector<Entity> &Entities() const { return m_denseEntities; }
	std::vector<Entity> &Entities() { return m_denseEntities; }

	size_t Size() const { return m_denseEntities.size(); }

private:
	void EnsureSparse(Entity e)
	{
		if (e < m_sparse.size())
			return;
		size_t newSize = static_cast<size_t>(e) + 1;
		m_sparse.resize(newSize, npos);
	}

	std::vector<size_t> m_sparse;
	std::vector<Entity> m_denseEntities;
	std::vector<T> m_denseComponents;
};