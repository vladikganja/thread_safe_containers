#pragma once
#include <lib/include/queues/classic_b_queue.hpp>
#include <lib/include/queues/lf_b_queue.hpp>

void foo100() {
    std::this_thread::sleep_for(1ms);
}

void foo1000() {
    std::this_thread::sleep_for(1000ms);
}

template <typename TaskT, template <typename> class QueueT>
class ThreadPool {
protected:
    uint64_t nThreads_;
    uint64_t nTasks_;

    mutable std::mutex taskMut_;

    QueueT<TaskT>& queue_;

    std::vector<std::thread> threads_;
    bool finished_ = false;

public:
    ThreadPool(uint64_t nThreads, uint64_t nTastks, QueueT<TaskT>& queue)
            : nThreads_(nThreads), nTasks_(nTastks), queue_(queue) {
    }

    virtual ~ThreadPool(){};
};

template <typename TaskT, template <typename> class QueueT>
class ProducerPool : public ThreadPool<TaskT, QueueT> {
private:
    using ThreadPool<TaskT, QueueT>::nTasks_;
    using ThreadPool<TaskT, QueueT>::nThreads_;
    using ThreadPool<TaskT, QueueT>::taskMut_;
    using ThreadPool<TaskT, QueueT>::queue_;
    using ThreadPool<TaskT, QueueT>::threads_;
    using ThreadPool<TaskT, QueueT>::finished_;

    bool check_finish() {
        std::lock_guard<std::mutex> guard{taskMut_};
        if (nTasks_ == 0) {
            return true;
        }
        nTasks_ -= 1;
        return false;
    }

    void produce() {
        for (;;) {
            if (check_finish())
                break;

            INFO("push()");

            auto&& [task, future] = createTask(foo100);

            if constexpr (std::is_same<QueueT<TaskT>, ClassicBQueue<TaskT>>::value) {
                queue_.enqueue(std::move(task));
            } else if (std::is_same<QueueT<TaskT>, LockFreeBQueue_basic<TaskT>>::value) {
                while (!queue_.enqueue(std::move(task))) {
                    std::this_thread::yield();
                }
            }

            // foo1000
        }

        INFO("producer thread finished");
    }

public:
    ProducerPool(uint64_t nProducers, uint64_t nTastks, QueueT<TaskT>& queue)
            : ThreadPool<TaskT, QueueT>(nProducers, nTastks, queue) {
    }

    ~ProducerPool() override {
        if (!finished_) {
            finish_produce();
        }
    }

    void start_produce() {
        for (uint64_t thread = 0; thread < nThreads_; thread++) {
            INFO("producer thread launched");
            threads_.emplace_back(&ProducerPool::produce, this);
        }
    }

    void finish_produce() {
        for (auto& thread : threads_) {
            thread.join();
        }

        if constexpr (std::is_same<QueueT<TaskT>, ClassicBQueue<TaskT>>::value) {
            queue_.doneWakeUp();
        }
        
        finished_ = true;
    }
};

template <typename TaskT, template <typename> class QueueT>
class ConsumerPool : ThreadPool<TaskT, QueueT> {
private:
    using ThreadPool<TaskT, QueueT>::nTasks_;
    using ThreadPool<TaskT, QueueT>::nThreads_;
    using ThreadPool<TaskT, QueueT>::taskMut_;
    using ThreadPool<TaskT, QueueT>::queue_;
    using ThreadPool<TaskT, QueueT>::threads_;
    using ThreadPool<TaskT, QueueT>::finished_;

    std::atomic<uint64_t> consumed_{};

    bool check_finish() {
        std::lock_guard<std::mutex> guard{taskMut_};

        if constexpr (std::is_same<QueueT<TaskT>, ClassicBQueue<TaskT>>::value) {
            if (consumed_ == nTasks_ && queue_.emptyAndDone()) {
                return true;
            }
        } else {
            if (consumed_ == nTasks_ && queue_.empty()) {
                return true;
            }
        }

        return false;
    }

    void consume() {
        for (;;) {
            if (check_finish())
                break;

            Task t;
            bool success = queue_.dequeue(t);
            if (success) {
                INFO("dequeue()");
                auto res = std::move(t)();
                consumed_++;
            }
        }

        INFO("consumer thread finished");
    }

public:
    ConsumerPool(uint64_t nConsumers, uint64_t nTasks, QueueT<TaskT>& queue)
            : ThreadPool<TaskT, QueueT>(nConsumers, nTasks, queue) {
    }

    ~ConsumerPool() override {
        if (!finished_) {
            finish_consume();
        }
    }

    void start_consume() {
        for (uint64_t thread = 0; thread < nThreads_; thread++) {
            INFO("consumer thread launched");
            threads_.emplace_back(&ConsumerPool::consume, this);
        }
    }

    void finish_consume() {
        for (auto& thread : threads_) {
            thread.join();
        }
        finished_ = true;
    }

    uint64_t consumed() const {
        return consumed_;
    }
};
