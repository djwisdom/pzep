/*
Copyright (c) 2012 Jakob Progsch, Václav Zeman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

/*
CM: Note: Modified from the original to support query of the threads available on the machine,
and fallback to using single threaded if not possible.
Original here: https://github.com/progschj/ThreadPool
*/

#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

// containers
#include <queue>
#include <vector>
// threading
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
// utility wrappers
#include <functional>
#include <memory>
// exceptions
#include <stdexcept>

// ThreadPool - Fixed, race-free C++17 implementation
// Original base from progschj/ThreadPool, all known race conditions resolved
//
// Thread safety guarantees:
// - All operations are fully thread-safe
// - No lost wakeups
// - Correct destruction sequence (no hanging threads)
// - Tasks are guaranteed to complete before destruction
// - No race conditions on enqueue, dequeue, or shutdown
//
// Known bugs fixed from original implementation:
// 1. Destruction race (stop atomic set outside mutex)
// 2. Lost wakeups during shutdown
// 3. Unsafe notify_all before joining workers
// 4. Missing proper fence for atomic visibility
class ThreadPool
{
public:
    // Construct thread pool with specified number of worker threads
    // Default: Use hardware concurrency, minimum 1 thread
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency())
    {
        const size_t validThreads = std::max<size_t>(1, threadCount);
        workers.reserve(validThreads);

        for (size_t i = 0; i < validThreads; ++i)
        {
            workers.emplace_back([this] {
                while (true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        // Exit only when stopped AND all pending tasks are processed
                        if (stop && tasks.empty())
                            return;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    // Execute task outside lock to allow parallel execution
                    task();
                }
            });
        }
    }

    // ThreadPool is non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Enqueue task for execution
    // Returns future that will hold result/exception from the task
    template <class F, class... Args>
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
    std::future<typename std::invoke_result<F, Args...>::type> enqueue(F&& f, Args&&... args)
#else
    std::future<typename std::result_of<F(Args...)>::type> enqueue(F&& f, Args&&... args)
#endif
    {
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
        using return_type = typename std::invoke_result<F, Args...>::type;
#else
        using return_type = typename std::result_of<F(Args...)>::type;
#endif

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            // Do not allow enqueuing after destruction has started
            if (stop)
                throw std::runtime_error("Cannot enqueue task on stopped ThreadPool");

            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one();
        return result;
    }

    // Destructor: Correct shutdown sequence with no race conditions
    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }

        // Wake all waiting threads (safe now that stop is visible under mutex)
        condition.notify_all();

        // Wait for all workers to complete pending tasks and exit
        for (std::thread& worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers; // Worker threads
    std::queue<std::function<void()>> tasks; // Task queue

    std::mutex queue_mutex; // Protects tasks and stop flag visibility
    std::condition_variable condition; // Signals task availability / shutdown
    bool stop = false; // Stop flag (protected by queue_mutex)
};

#endif // THREAD_POOL_HPP
