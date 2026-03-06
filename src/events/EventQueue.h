/**
 * @file EventQueue.h
 * @brief Declarations for a typed POD event queue.
 */

#pragma once

#pragma region Includes
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#pragma endregion

#pragma region Declarations
namespace events
{
	namespace detail
	{
		template <typename T, typename... Ts>
		inline constexpr bool IsOneOfV = (std::is_same_v<T, Ts> || ...);

		template <typename T>
		inline constexpr bool IsPodEventV = std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>;
	}

	template <typename... EventTypes>
	class EventQueue
	{
	public:
#pragma region Public Interface
		template <typename T>
		void Push(const T &event)
		{
			static_assert(detail::IsOneOfV<T, EventTypes...>, "Event type is not registered in this EventQueue.");
			static_assert(detail::IsPodEventV<T>, "Event type must be POD-compatible.");
			std::get<std::vector<T>>(m_events).push_back(event);
		}

		template <typename T, typename Callback>
		void ConsumeAll(Callback &&callback)
		{
			static_assert(detail::IsOneOfV<T, EventTypes...>, "Event type is not registered in this EventQueue.");
			static_assert(detail::IsPodEventV<T>, "Event type must be POD-compatible.");

			std::vector<T> &events = std::get<std::vector<T>>(m_events);
			if (events.empty())
				return;

			std::vector<T> pending;
			pending.swap(events);
			for (const T &event : pending)
				callback(event);
		}

		template <typename T>
		const std::vector<T> &Get() const
		{
			static_assert(detail::IsOneOfV<T, EventTypes...>, "Event type is not registered in this EventQueue.");
			static_assert(detail::IsPodEventV<T>, "Event type must be POD-compatible.");
			return std::get<std::vector<T>>(m_events);
		}

		template <typename T>
		std::vector<T> &Get()
		{
			static_assert(detail::IsOneOfV<T, EventTypes...>, "Event type is not registered in this EventQueue.");
			static_assert(detail::IsPodEventV<T>, "Event type must be POD-compatible.");
			return std::get<std::vector<T>>(m_events);
		}

		template <typename T>
		void Clear()
		{
			static_assert(detail::IsOneOfV<T, EventTypes...>, "Event type is not registered in this EventQueue.");
			static_assert(detail::IsPodEventV<T>, "Event type must be POD-compatible.");
			std::get<std::vector<T>>(m_events).clear();
		}

		void ClearAll()
		{
			std::apply([](auto &...queues)
					   { (queues.clear(), ...); }, m_events);
		}
#pragma endregion

	private:
		std::tuple<std::vector<EventTypes>...> m_events;
	};
}
#pragma endregion
