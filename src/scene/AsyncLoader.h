/**
 * @file AsyncLoader.h
 * @brief Declarations for AsyncLoader.
 */

#pragma once

#pragma region Includes
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>
#pragma endregion

#pragma region Declarations
class AsyncLoader
{
public:
	#pragma region Public Interface
    /**
     * @brief Constructs a new AsyncLoader instance.
     */
    AsyncLoader() = default;

    /**
     * @brief Executes Enqueue.
     */
    template <class T>
    void Enqueue(std::function<T()> backgroundTask, std::function<void(T &&)> mainThreadFinalize)
    {
        struct Pending final : PendingBase
        {
            std::future<T> fut;
            std::function<void(T &&)> finalize;

            Pending(std::future<T> &&f, std::function<void(T &&)> &&fn)
                : fut(std::move(f)), finalize(std::move(fn))
            {
            }

            bool TryComplete(std::vector<std::function<void()>> &outMain) override
            {
                if (fut.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
                    return false;

                T value = fut.get();

                outMain.emplace_back([fn = std::move(finalize), v = std::move(value)]() mutable
                                     { fn(std::move(v)); });

                return true;
            }
        };

        auto fut = std::async(std::launch::async, std::move(backgroundTask));

        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending.emplace_back(std::make_unique<Pending>(std::move(fut), std::move(mainThreadFinalize)));
    }

    /**
     * @brief Executes Update.
     */
    void Update()
    {
        std::vector<std::function<void()>> mainLocal;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            for (size_t i = 0; i < m_pending.size();)
            {
                if (m_pending[i]->TryComplete(mainLocal))
                {
                    m_pending[i] = std::move(m_pending.back());
                    m_pending.pop_back();
                    continue;
                }
                ++i;
            }
        }

        for (auto &fn : mainLocal)
            fn();
    }

    /**
     * @brief Executes Is Idle.
     */
    bool IsIdle() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pending.empty();
    }

	#pragma endregion
private:
	#pragma region Private Implementation
    struct PendingBase
    {
        virtual ~PendingBase() = default;
        virtual bool TryComplete(std::vector<std::function<void()>> &outMain) = 0;
    };

	#pragma endregion
private:
	#pragma region Private Implementation
    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<PendingBase>> m_pending;
	#pragma endregion
};
#pragma endregion
