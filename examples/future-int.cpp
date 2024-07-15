#include <aco/thread_pool.hpp>

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

std::function<int()> TaskFactory();

int main() {
    using std::chrono::operator""ms;
    aco::thread_pool tp{4, 50ms};
    size_t const n_tasks = 1000;
    std::vector<std::future<int>> futures;
    futures.reserve(n_tasks);
    std::cout << "Pushing tasks ..." << std::endl;
    for (size_t n = 0; n < n_tasks; ++n) {
        auto task = TaskFactory();
        auto future = tp.submit(task);
        futures.push_back(std::move(future));
    }
    std::cout << "Waiting for completion ..." << std::endl;
    int result = 0;
    for (auto& f : futures) {
        result += f.get();
    }
    std::cout << "Result: " << result << "\n";
    return 0;
}

std::function<int()> TaskFactory() {
    int const v = ::rand(); // NOLINT
    return [v] {
        using std::chrono::operator""ms;
        std::this_thread::sleep_for(10ms);
        return v;
    };
}
