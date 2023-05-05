#pragma once

#include <lib/queues/classic_b_queue.hpp>
#include <lib/queues/lf_b_queue.hpp>

template <typename TaskT, template <typename> class QueueT>
class ThreadPool {
private:
    uint64_t nWorkers_;
    volatile std::atomic<uint64_t> maxDepthTasks_;

    QueueT<TaskT> queue_;
    std::vector<std::thread> workers_;

    void workerRoutine() {
        while (!queue_.emptyAndDone()) {
            Task t;
            bool success = queue_.dequeue(t);
            if (success) {
                auto res = std::move(t)();
            } else {
                std::this_thread::yield();
            }
        }
    }

    void startWorkers() {
        for (uint64_t worker = 0; worker < nWorkers_; ++worker) {
            workers_.emplace_back([this]() {
                workerRoutine();
            });
        }
    }

public:
    ThreadPool(uint64_t nWorkers, uint64_t maxDepthTasks)
            : nWorkers_(nWorkers),
              maxDepthTasks_(maxDepthTasks),
              queue_(1024) {
        startWorkers();
    }

    ~ThreadPool() {
        assert(queue_.emptyAndDone() && workers_.empty());
    }

    void maxDepthReached() {
        maxDepthTasks_.fetch_sub(1, std::memory_order_relaxed);
    }

    void submit(TaskT&& task) {
        queue_.enqueue(std::move(task));
    }

    void wait() {
        while (maxDepthTasks_.load() > 0) {
            std::this_thread::yield();
        }
        queue_.doneWakeUp();
    }

    void stop() {
        for (auto& worker: workers_) {
            worker.join();
        }
        workers_.clear();
    }
};
