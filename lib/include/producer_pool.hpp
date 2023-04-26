#pragma once
#include "task.h"
#include "classic_b_queue.hpp"

int foo1(int a, int b) {
    return a ^ b;
}

std::string foo2(const std::string& s1, const std::string& s2) {
    return s1 + s2;
}

template <typename T>
class ProducerPool {
private:
    std::atomic<uint64_t> nTasks_;
    uint64_t nProducers_;

    ClassicBQueue<T>& queue_;

    std::vector<std::thread> producers_;

    // this job executes in single thread
    void producer_job() {
        while (nTasks_) {
            if (nTasks_.load() % 2 == 0) {
                auto task = Task::createTask(foo1, 42, 23);
                queue_.push(task);
            } else {
                auto task = Task::createTask(foo2, "42", "23");
                queue_.push(task);
            }
        }
    }

public:
    ProducerPool(uint64_t nProducers, uint64_t nTastks, ClassicBQueue<T>& queue)
            : nTasks_(nTastks), nProducers_(nProducers), queue_(queue) {
    }

    ~ProducerPool() {
        for (auto& thread: producers_) {
            thread.join();
        }
    }

    void start_produce() {
        for (uint64_t thread = 0; thread < nProducers_; thread++) {
            producers_.emplace_back(producer_job);
        }
    }
};
