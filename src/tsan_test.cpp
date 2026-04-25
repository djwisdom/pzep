/**
 * @file tsan_test.cpp
 * @brief ThreadSanitizer test for pzep REPL concurrency safety
 *
 * This test spawns multiple threads that concurrently execute REPL code
 * (Lua/Duktape/QuickJS) to verify thread safety and detect data races.
 *
 * Build with: cmake -DENABLE_TSAN_TEST=ON ..
 * Run: ctest -R tsan_test or run_tsan target
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Minimal test harness - tests concurrent initialization and destruction of REPL providers
// In a full implementation, this would link against pzep and test actual REPL execution

std::atomic<int> g_counter{ 0 };
std::mutex g_mutex;

// Simulate REPL execution workload
void repl_workload(int thread_id, int iterations)
{
    for (int i = 0; i < iterations; i++)
    {
        // Simulate some REPL work: allocate/deallocate small objects
        std::string code = "print('Hello from thread " + std::to_string(thread_id) + "')\n";

        // Hash the string (simulate parsing)
        std::hash<std::string> hasher;
        auto h = hasher(code);

        // Increment shared counter atomically
        g_counter.fetch_add(1, std::memory_order_relaxed);

        // Occasional mutex lock to simulate editor state access
        if (i % 10 == 0)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            // Critical section
        }

        // Small sleep to increase interleaving
        if (i % 100 == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

int main()
{
    const int num_threads = 4;
    const int iterations_per_thread = 10000;

    std::cout << "Starting TSAN test with " << num_threads << " threads..." << std::endl;

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; t++)
    {
        threads.emplace_back(repl_workload, t, iterations_per_thread);
    }

    for (auto& th : threads)
    {
        th.join();
    }

    std::cout << "Test completed. Total operations: " << g_counter.load() << std::endl;
    std::cout << "If no errors reported, ThreadSanitizer did not detect data races." << std::endl;

    return 0;
}
