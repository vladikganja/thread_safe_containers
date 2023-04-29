#pragma once
#include "task.hpp"
#include "classic_b_queue.hpp"

int foo1(int a, int b) {
    return a ^ b;
}

std::string foo2(const std::string& s1, const std::string& s2) {
    return s1 + s2;
}

template <typename TaskT>
class ProducerPool {
private:
    uint64_t nTasks_;
    uint64_t nThreads_;

    mutable std::mutex taskMut_;

    ClassicBQueue<TaskT>& queue_;

    std::vector<std::thread> threads_;

    void run() {
        for (;;) {
            std::unique_lock<std::mutex> guard{taskMut_};
            if (nTasks_ == 0) {
                break;
            }
            nTasks_ -= 1;
            guard.unlock();

            if (nTasks_ % 2 == 0) {
                auto&& [task, future] = createTask(foo1, 42, 23);
                queue_.push(std::move(task));
            }
            else {
                auto&& [task, future] = createTask(foo2, "42", "23");
                queue_.push(std::move(task));
            }
        }
    }

public:
    ProducerPool(uint64_t nProducers, uint64_t nTastks, ClassicBQueue<TaskT>& queue)
            : nTasks_(nTastks), nThreads_(nProducers), queue_(queue) {
    }

    ~ProducerPool() {
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    void start() {
        for (uint64_t thread = 0; thread < nThreads_; thread++) {
            threads_.emplace_back(&ProducerPool::run, this);
        }
    }
};
