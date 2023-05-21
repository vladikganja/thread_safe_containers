#pragma once

#include <lib/queues/blocking_bounded_queue.hpp>    // BB
#include <lib/queues/blocking_unbounded_queue.hpp>  // BU
#include <lib/queues/lockfree_bounded_queue.hpp>    // LFB
#include <lib/queues/lockfree_unbounded_queue.hpp>  // LFU
#include <lib/queues/waitfree_unbounded_queue.hpp>  // WFU

#include <lib/queues/uniQueue.hpp>

template <typename TaskT, typename QueueT>
class ThreadPool {
private:
    uint64_t nWorkers_;
    volatile std::atomic<uint64_t> maxDepthTasks_;

    QueueT queue_;
    std::vector<std::thread> workers_;

    std::atomic<bool> allTasksSubmitted_;

    void workerRoutine() {
        while (!allTasksSubmitted_ || !queue_.empty()) {
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
        // The same for all types of queues
        for (uint64_t worker = 0; worker < nWorkers_; ++worker) {
            workers_.emplace_back([this]() {
                workerRoutine();
            });
        }
    }

public:
    ThreadPool(uint64_t nWorkers, uint64_t maxDepthTasks)
            : nWorkers_(nWorkers),
              maxDepthTasks_(maxDepthTasks)
              /*,
              queue_((2 << static_cast<uint64_t>(log2(maxDepthTasks))))*/ {
        startWorkers();
    }

    ~ThreadPool() {
        REQUIRE(allTasksSubmitted_ && queue_.empty() && workers_.empty(),
                "Thread pool not finished");
    }

    void maxDepthReached() {
        maxDepthTasks_.fetch_sub(1, std::memory_order_relaxed);
    }

    void submit(TaskT&& task) {
        queue_.enqueue(std::move(task));
    }

    void join() {
        while (maxDepthTasks_.load() > 0) {
            std::this_thread::yield();
        }

        allTasksSubmitted_.store(true);
        //queue_.wakeUp();

        for (auto& worker : workers_) {
            worker.join();
        }

        workers_.clear();
    }
};
