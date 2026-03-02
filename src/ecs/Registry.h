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

/**
 * @brief Central entity/component registry for the ECS.
 *
 * Manages creation and destruction of entities and stores component pools
 * for arbitrary types. Provides utilities to query entity lifetimes and
 * to view entities that possess particular component sets.
 *
 * @note All public member functions are linear or better in the number of
 * entities/components; heavy use of templated methods results in inline
 * code for each component type.
 */
class Registry
{
public:
	/**
	 * @brief Create a new entity handle.
	 *
	 * Reuses IDs from the free list when possible; otherwise increments the
	 * internal counter. Newly created entities are considered alive and are
	 * appended to the alive vector.
	 *
	 * @return Entity A fresh entity identifier.
	 */
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

	/**
	 * @brief Destroy an entity and remove all associated components.
	 *
	 * The entity is removed from the alive list and added to the free list for
	 * future reuse. Every component pool is notified to remove the entity if
	 * it exists there.
	 *
	 * @param e The entity to destroy; behaviour is undefined if the entity is
	 * not currently alive.
	 */
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

	/**
	 * @brief Construct or update a component of type T for an entity.
	 *
	 * Forwards arguments to the component's constructor. If the component pool
	 * does not yet exist it is created.
	 *
	 * @tparam T  Component type.
	 * @tparam Args  Constructor argument types for T.
	 * @param e Entity to associate with the component.
	 * @param args Arguments forwarded to T's constructor.
	 * @return T& Reference to the inserted component.
	 */
	template <typename T, typename... Args>
	T &Emplace(Entity e, Args &&...args)
	{
		return Pool<T>().Emplace(e, std::forward<Args>(args)...);
	}

	/**
	 * @brief Check if an entity has a component of type T.
	 *
	 * @tparam T Component type.
	 * @param e Entity handle.
	 * @return true if the entity owns a T component.
	 */
	template <typename T>
	bool Has(Entity e) const
	{
		return Pool<T>().Has(e);
	}

	/**
	 * @brief Get a mutable reference to an entity's component.
	 *
	 * @tparam T Component type.
	 * @param e Entity handle.
	 * @return T& Reference to the component instance.
	 *
	 * @note Behavior is undefined if the component does not exist; use `Has`
	 *       to check beforehand.
	 */
	template <typename T>
	T &Get(Entity e)
	{
		return Pool<T>().Get(e);
	}

	/**
	 * @brief Get a const reference to an entity's component.
	 *
	 * @tparam T Component type.
	 * @param e Entity handle.
	 * @return const T& Const reference to the component instance.
	 */
	template <typename T>
	const T &Get(Entity e) const
	{
		return Pool<T>().Get(e);
	}

	/**
	 * @brief Retrieve a list of currently alive entities.
	 *
	 * @return const std::vector<Entity>& Vector of live entity IDs.
	 */
	const std::vector<Entity> &Alive() const { return m_alive; }

	/**
	 * @brief Populate a list of entities that have all specified components.
	 *
	 * Scans the smallest component pool among the given types and filters
	 * entities by verifying presence in each remaining pool. Useful for
	 * iterating over entities with specific component combinations.
	 *
	 * @tparam Ts Component types to filter by.
	 * @param out Output vector that will be filled with matching entities.
	 */
	template <typename... Ts>
	void ViewEntities(std::vector<Entity> &out) const
	{
		const auto &smallest = SmallestPoolEntities<Ts...>();
		out.clear();
		out.reserve(smallest.size());
		for (Entity e : smallest)
		{
			if ((Pool<Ts>().Has(e) && ...))
				out.push_back(e);
		}
	}

private:
	/**
	 * @brief Abstract interface for component pools.
	 *
	 * Used internally to store heterogeneous component sets in a type-erased
	 * map. Enables generic operations like removal and querying size.
	 */
	struct IPool
	{
		virtual ~IPool() = default;
		virtual void RemoveEntity(Entity e) = 0;
		virtual size_t Size() const = 0;
		virtual const std::vector<Entity> &Entities() const = 0;
	};

	/**
	 * @brief Concrete pool implementation for a specific component type.
	 *
	 * Uses `SparseSet<T>` to store components efficiently by entity handle.
	 * Implements the IPool interface so it can be stored in the generic map.
	 */
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

	/**
	 * @brief Obtain a mutable reference to the pool for type `T`, creating it
	 *        if necessary.
	 *
	 * This performs type-erasure via `std::type_index` and a map of
	 * `unique_ptr<IPool>`. The returned reference is safe to use for
	 * component operations.
	 */
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

	/**
	 * @brief Const version of `Pool()`; returns an empty pool reference if none
	 *        exists for type `T`.
	 */
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

	/**
	 * @brief Return the entity list of the smallest component pool among `Ts`.
	 *
	 * Used internally by `ViewEntities` to optimize intersection. The pool
	 * that reports the fewest entities is chosen as the candidate set.
	 */
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

	// Next entity ID to allocate when free list is empty
	Entity m_next = 0;

	// Recycled entity IDs available for reuse
	std::vector<Entity> m_free;

	// Currently alive entities
	std::vector<Entity> m_alive;

	// Map of component pools keyed by type_index
	std::unordered_map<std::type_index, std::unique_ptr<IPool>> m_pools;
};