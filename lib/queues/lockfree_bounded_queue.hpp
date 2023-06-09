#pragma once

#include <lib/common/task.hpp>

template <typename T>
class mpmc_bounded_queue {
private:
    struct Cell {
        std::atomic<uint64_t> sequence;
        TaskT task;
    };

    std::vector<Cell> buffer_;
    uint64_t bufMask_;
    std::atomic<uint64_t> enqueuePos_;
    std::atomic<uint64_t> dequeuePos_;

public:
    LockfreeBoundedQueue(uint64_t size = 128): buffer_(size), bufMask_(size - 1) {
        for (uint64_t i = 0; i < size; i++) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }

        enqueuePos_.store(0, std::memory_order_relaxed);
        dequeuePos_.store(0, std::memory_order_relaxed);
    }

    void enqueue(TaskT&& task) {
        Cell* cell;
        uint64_t pos;
        bool res = false;

        while (!res) {
            // fetch the current Position where to enqueue the item
            pos = enqueuePos_.load(std::memory_order_relaxed);
            cell = &buffer_[pos & bufMask_];
            auto seq = cell->sequence.load(std::memory_order_acquire);
            auto diff = static_cast<int>(seq) - static_cast<int>(pos);

            // queue is full we cannot enqueue and just continue
            // another option: queue moved forward all way round
            if (diff < 0) {
                continue;
            }

            // If its Sequence wasn't touched by other producers
            // check if we can increment the enqueue Position
            if (diff == 0) {
                res = enqueuePos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed);
            }
        }

        // write the item we want to enqueue and bump Sequence
        cell->task = std::move(task);
        cell->sequence.store(pos + 1, std::memory_order_release);
    }

    bool dequeue(TaskT& task) {
        Cell* cell;
        uint64_t pos;
        bool res = false;

        while (!res) {
            // fetch the current Position from where we can dequeue an item
            pos = dequeuePos_.load(std::memory_order_relaxed);
            cell = &buffer_[pos & bufMask_];
            auto seq = cell->sequence.load(std::memory_order_acquire);
            auto diff = static_cast<int>(seq) - static_cast<int>(pos + 1);

            // probably the queue is empty, then return false
            if (diff < 0) {
                return false;
            }

            // Check if its Sequence was changed by a producer and wasn't changed by
            // other consumers and if we can increment the dequeue Position
            if (diff == 0) {
                res = dequeuePos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed);
            }
        }

        // read the item and update for the next round of the buffer
        task = std::move(cell->task);
        cell->sequence.store(pos + bufMask_ + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return enqueuePos_.load() == dequeuePos_.load();
    }
};
