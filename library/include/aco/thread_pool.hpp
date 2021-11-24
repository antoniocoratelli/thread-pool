// Copyright 2021 Antonio Coratelli
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ACO_THREAD_POOL_HPP_INCLUDED_
#define ACO_THREAD_POOL_HPP_INCLUDED_

#if __cplusplus < 201703L
#    error "aco::thread_pool requires C++17 or later."
#endif

#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

namespace aco {

/// A minimalist implementation of a C++17 thread-pool-based task scheduler with
/// task-stealing, in around 200 lines of header-only code with no external 
/// dependencies.
///
/// The implementation is based on the task scheduler system described by
/// [Sean Parent](https://sean-parent.stlab.cc/) in his legendary talk
/// [Better Code: Concurrency](https://youtu.be/zULU6Hhp42w?t=1583).
///
/// \tparam TaskStealingRounds: number of rounds of task stealing.
///         If 0, the task-stealing feature will be disabled at compile-time.
template<size_t TaskStealingRounds = 4>
class thread_pool {
public:
    template<typename ReturnType>
    using task_type = std::function<ReturnType()>;

    /// Constructs an aco::thread_pool with the desired amount of threads.
    /// If `n_threads` is 0, the thread_pool will instantiate a number
    /// of threads equal to the hardware concurrency level of the machine.
    explicit thread_pool(size_t n_threads = 0);

    ~thread_pool() noexcept;
    thread_pool(thread_pool const&) = delete;
    thread_pool(thread_pool&&) noexcept = delete;
    thread_pool& operator=(thread_pool const&) = delete;
    thread_pool& operator=(thread_pool&&) noexcept = delete;

    /// Pushes some work to the thread_pool.
    void push(task_type<void> const& task);

    /// Pushes some work to the thread_pool.
    /// Returns a future that can be used to wait for the completion of the
    /// given task, and get the work result (or the eventual exception thrown).
    template<typename ReturnType>
    auto submit(task_type<ReturnType> const& task);

private:
    class queue {
    public:
        queue();
        bool pop(task_type<void>& task);
        void push(task_type<void> const& task);
        bool try_pop(task_type<void>& task);
        bool try_push(task_type<void> const& task);
        void done();

    private:
        using lock_type = std::unique_lock<std::mutex>;
        std::deque<task_type<void>> queue_;
        bool done_;
        std::mutex mutex_;
        std::condition_variable ready_;
    };

    std::vector<std::thread> threads_;
    std::vector<queue> queues_;
    std::atomic_size_t counter_;

    void run_(size_t thread_index);
    static size_t get_n_threads_(size_t n_threads);
};

template<size_t NumRounds>
thread_pool<NumRounds>::thread_pool(size_t n_threads):
    queues_{get_n_threads_(n_threads)}, counter_{0} {
    threads_.reserve(queues_.size());
    for (size_t n = 0; n < queues_.size(); ++n) {
        threads_.emplace_back([this, n] { this->run_(n); });
    }
}

template<size_t NumRounds>
thread_pool<NumRounds>::~thread_pool() noexcept {
    auto call_done = std::mem_fn(&queue::done);
    auto call_join = std::mem_fn(&std::thread::join);
    std::for_each(begin(queues_), end(queues_), call_done);
    std::for_each(begin(threads_), end(threads_), call_join);
}

template<size_t NumRounds>
void thread_pool<NumRounds>::push(task_type<void> const& task) {
    size_t const count = counter_++;
    if constexpr (NumRounds > 0) {
        size_t const n_max = queues_.size() * NumRounds;
        for (size_t n = 0; n < n_max; ++n) {
            size_t const index = (count + n) % queues_.size();
            if (queues_[index].try_push(task)) return;
        }
    }
    size_t const index = count % queues_.size();
    queues_[index].push(task);
}

template<size_t NumRounds>
template<typename ReturnType>
auto thread_pool<NumRounds>::submit(task_type<ReturnType> const& task) {
    using promise_type = std::promise<ReturnType>;
    auto promise = std::make_shared<promise_type>();
    auto future = promise->get_future();
    this->push([task_ = task, promise_ = std::move(promise)]() {
        try {
            if constexpr (std::is_same_v<void, ReturnType>) {
                task_();
                promise_->set_value();
            } else {
                promise_->set_value(task_());
            }
        } catch (...) {
            promise_->set_exception(std::current_exception());
        }
    });
    return future;
}

template<size_t NumRounds>
void thread_pool<NumRounds>::run_(size_t thread_index) {
    while (true) {
        task_type<void> task;
        if constexpr (NumRounds > 0) {
            for (size_t n = 0; n < queues_.size(); ++n) {
                size_t const index = (thread_index + n) % queues_.size();
                if (queues_[index].try_pop(task)) break;
            }
        }
        if (not task and not queues_[thread_index].pop(task)) break;
        assert(task);
        task();
    }
}

template<size_t NumRounds>
size_t thread_pool<NumRounds>::get_n_threads_(size_t n_threads) {
    if (n_threads == 0) {
        size_t const hwc = std::thread::hardware_concurrency();
        return hwc > 0 ? hwc : 1;
    }
    return n_threads;
}

template<size_t NumRounds>
thread_pool<NumRounds>::queue::queue(): done_{false} {
}

template<size_t NumRounds>
bool thread_pool<NumRounds>::queue::pop(task_type<void>& task) {
    auto lock = lock_type{mutex_};
    while (queue_.empty() and not done_) {
        using std::chrono::operator""ms;
        ready_.wait_for(lock, 250ms);
    }
    if (queue_.empty()) return false;
    task = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

template<size_t NumRounds>
void thread_pool<NumRounds>::queue::push(task_type<void> const& task) {
    {
        auto lock = lock_type{mutex_};
        queue_.push_back(task);
    }
    ready_.notify_one();
}

template<size_t NumRounds>
bool thread_pool<NumRounds>::queue::try_pop(task_type<void>& task) {
    auto lock = lock_type{mutex_, std::try_to_lock};
    if (not lock or queue_.empty()) return false;
    task = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

template<size_t NumRounds>
bool thread_pool<NumRounds>::queue::try_push(task_type<void> const& task) {
    {
        auto lock = lock_type{mutex_, std::try_to_lock};
        if (not lock) return false;
        queue_.push_back(task);
    }
    ready_.notify_one();
    return true;
}

template<size_t NumRounds>
void thread_pool<NumRounds>::queue::done() {
    {
        auto lock = lock_type{mutex_};
        done_ = true;
    }
    ready_.notify_all();
}

} // namespace aco

#endif
