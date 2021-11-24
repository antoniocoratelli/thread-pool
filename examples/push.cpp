#include <aco/thread_pool.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

std::atomic_int counter{0};
std::function<void()> TaskFactory(std::atomic_int* counter_ptr);

int main() {
    {
        aco::thread_pool tp{4};
        size_t const n_tasks = 1000;
        std::cout << "Pushing tasks ..." << std::endl;
        for (size_t n = 0; n < n_tasks; ++n) {
            auto task = TaskFactory(&counter);
            tp.push(task);
        }
        std::cout << "Waiting for completion ..." << std::endl;
    }
    std::cout << "Result: " << counter << std::endl;
    return 0;
}

std::function<void()> TaskFactory(std::atomic_int* counter_ptr) {
    int const v = ::rand(); // NOLINT
    return [v, counter_ptr] {
        using std::chrono::operator""ms;
        std::this_thread::sleep_for(10ms);
        counter_ptr->fetch_add(v);
    };
}
