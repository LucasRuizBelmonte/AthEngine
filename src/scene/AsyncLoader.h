#pragma once

#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>

class AsyncLoader
{
public:
    AsyncLoader() = default;

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

    bool IsIdle() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pending.empty();
    }

private:
    struct PendingBase
    {
        virtual ~PendingBase() = default;
        virtual bool TryComplete(std::vector<std::function<void()>> &outMain) = 0;
    };

private:
    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<PendingBase>> m_pending;
};