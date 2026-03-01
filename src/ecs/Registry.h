#pragma once
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include <tuple>
#include <utility>
#include <algorithm>

#include "Entity.h"
#include "SparseSet.h"

class Registry
{
public:
	Entity Create()
	{
		Entity e;
		if (!m_free.empty())
		{
			e = m_free.back();
			m_free.pop_back();
		}
		else
		{
			e = m_next++;
		}
		m_alive.push_back(e);
		return e;
	}

	void Destroy(Entity e)
	{
		for (auto &kv : m_pools)
			kv.second->RemoveEntity(e);

		auto it = std::find(m_alive.begin(), m_alive.end(), e);
		if (it != m_alive.end())
		{
			*it = m_alive.back();
			m_alive.pop_back();
		}
		m_free.push_back(e);
	}

	template <typename T, typename... Args>
	T &Emplace(Entity e, Args &&...args)
	{
		return Pool<T>().Emplace(e, std::forward<Args>(args)...);
	}

	template <typename T>
	bool Has(Entity e) const
	{
		return Pool<T>().Has(e);
	}

	template <typename T>
	T &Get(Entity e)
	{
		return Pool<T>().Get(e);
	}

	template <typename T>
	const T &Get(Entity e) const
	{
		return Pool<T>().Get(e);
	}

	const std::vector<Entity> &Alive() const { return m_alive; }

	template <typename... Ts>
	std::vector<Entity> ViewEntities() const
	{
		const auto &smallest = SmallestPoolEntities<Ts...>();
		std::vector<Entity> out;
		out.reserve(smallest.size());
		for (Entity e : smallest)
		{
			if ((Pool<Ts>().Has(e) && ...))
				out.push_back(e);
		}
		return out;
	}

private:
	struct IPool
	{
		virtual ~IPool() = default;
		virtual void RemoveEntity(Entity e) = 0;
		virtual size_t Size() const = 0;
		virtual const std::vector<Entity> &Entities() const = 0;
	};

	template <typename T>
	struct PoolImpl final : IPool
	{
		SparseSet<T> set;

		template <typename... Args>
		T &Emplace(Entity e, Args &&...args) { return set.Emplace(e, std::forward<Args>(args)...); }

		bool Has(Entity e) const { return set.Has(e); }

		T &Get(Entity e) { return set.Get(e); }
		const T &Get(Entity e) const { return set.Get(e); }

		void RemoveEntity(Entity e) override { set.Remove(e); }
		size_t Size() const override { return set.Size(); }
		const std::vector<Entity> &Entities() const override { return set.Entities(); }
	};

	template <typename T>
	PoolImpl<T> &Pool()
	{
		std::type_index key = std::type_index(typeid(T));
		auto it = m_pools.find(key);
		if (it == m_pools.end())
		{
			auto ptr = std::make_unique<PoolImpl<T>>();
			auto *raw = ptr.get();
			m_pools.emplace(key, std::move(ptr));
			return *raw;
		}
		return *static_cast<PoolImpl<T> *>(it->second.get());
	}

	template <typename T>
	const PoolImpl<T> &Pool() const
	{
		std::type_index key = std::type_index(typeid(T));
		auto it = m_pools.find(key);
		if (it == m_pools.end())
		{
			static PoolImpl<T> empty;
			return empty;
		}
		return *static_cast<const PoolImpl<T> *>(it->second.get());
	}

	template <typename... Ts>
	const std::vector<Entity> &SmallestPoolEntities() const
	{
		const IPool *pools[] = {&Pool<Ts>()...};
		const IPool *smallest = pools[0];
		for (auto *p : pools)
		{
			if (p->Size() < smallest->Size())
				smallest = p;
		}
		return smallest->Entities();
	}

	Entity m_next = 0;
	std::vector<Entity> m_free;
	std::vector<Entity> m_alive;
	std::unordered_map<std::type_index, std::unique_ptr<IPool>> m_pools;
};