#pragma once

#include <lib/queues/blocking_bounded_queue.hpp>    // BB
#include <lib/queues/blocking_unbounded_queue.hpp>  // BU
#include <lib/queues/lockfree_bounded_queue.hpp>    // LFB
#include <lib/queues/lockfree_unbounded_queue.hpp>  // LFU
#include <lib/queues/waitfree_unbounded_queue.hpp>  // WFU

template <typename TaskT, template <typename> class QueueT>
class ThreadPool {
private:
    uint64_t nWorkers_;
    volatile std::atomic<uint64_t> maxDepthTasks_;

    QueueT<TaskT> queue_;
    std::vector<std::thread> workers_;

    std::atomic<bool> allTasksSubmitted_;

    void workerRoutine() {
        if constexpr (std::is_same<QueueT<TaskT>, BlockingBoundedQueue<TaskT>>::value) {
        } else if constexpr (std::is_same<QueueT<TaskT>, BlockingUnboundedQueue<TaskT>>::value) {
            while (!allTasksSubmitted_ || !queue_.empty()) {
                Task t;
                bool success = queue_.dequeue(t);
                if (success) {
                    auto res = std::move(t)();
                } else {
                    std::this_thread::yield();
                }
            }
        } else if constexpr (std::is_same<QueueT<TaskT>, LockfreeBoundedQueue<TaskT>>::value) {
            while (!allTasksSubmitted_ || !queue_.empty()) {
                Task t;
                bool success = queue_.dequeue(t);
                if (success) {
                    auto res = std::move(t)();
                } else {
                    std::this_thread::yield();
                }
            }
        } else if (std::is_same<QueueT<TaskT>, LockfreeUnboundedQueue<TaskT>>::value) {
        } else if (std::is_same<QueueT<TaskT>, WaitfreeUnboundedQueue<TaskT>>::value) {
        } else {
            assert(false);
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
            : nWorkers_(nWorkers), maxDepthTasks_(maxDepthTasks) {
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

        for (auto& worker : workers_) {
            worker.join();
        }

        workers_.clear();
    }
};
