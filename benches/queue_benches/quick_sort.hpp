#pragma once

#include <benches/queue_benches/qs_thread_pool.hpp>

namespace bench {

// Многопоточный квиксорт, который имплементирует внутри логику
// тред-пула, построенного на синхронизированной очереди задач.

namespace {

namespace impl {

std::optional<ThreadPool<Task, LockfreeBoundedQueue>> pool;

template <typename Ty>
inline Ty median(Ty val1, Ty val2, Ty val3) {
    if (val1 > val2 && val2 > val3)
        return val2;
    else if (val2 > val1 && val1 > val3)
        return val2;
    return val3;
}

template <typename RanIt, typename CmpT>
inline RanIt partition(RanIt left, RanIt right, CmpT cmp) {
    auto pivot = impl::median(*left, *(left + (right - left) / 2), *--right);

    for (;;) {
        while (cmp(*left, pivot)) {
            ++left;
        }

        while (cmp(pivot, *right)) {
            --right;
        }

        if (left >= right) {
            break;
        }

        std::swap(*left, *right);
        ++left;
        --right;
    }

    return left;
}

template <typename RanIt, typename CmpT>
void task(RanIt begin, RanIt end, CmpT cmp) {
    if (begin == end - 1) {
        impl::pool->maxDepthReached();
        return;
    }

    RanIt mid = impl::partition(begin, end, cmp);

    auto&& [lTask, lFut] = createTask(task<RanIt, CmpT>, begin, mid, cmp);
    auto&& [rTask, rFut] = createTask(task<RanIt, CmpT>, mid, end, cmp);
    impl::pool->submit(std::move(lTask));
    impl::pool->submit(std::move(rTask));
}

template <typename RanIt, typename CmpT>
void singleThreadSort(RanIt begin, RanIt end, CmpT cmp) {
    if (begin == end - 1) {
        return;
    }

    RanIt mid = impl::partition(begin, end, cmp);

    impl::singleThreadSort(begin, mid, cmp);
    impl::singleThreadSort(mid, end, cmp);
}

}  // impl

} // namespace

template <typename RanIt, typename CmpT = std::less<>>
void quickSort(uint64_t threads, RanIt begin, RanIt end, CmpT cmp = CmpT()) {
    REQUIRE(threads > 0 && threads <= std::thread::hardware_concurrency(),
            "Invalid number of threads");

    if (threads == 1) {
        impl::singleThreadSort(begin, end, cmp);
        return;
    }

    impl::pool.emplace(threads, std::distance(begin, end));

    auto&& [initTask, fut] = createTask(impl::task<RanIt, CmpT>, begin, end, cmp);
    impl::pool->submit(std::move(initTask));

    impl::pool->join();

    impl::pool.reset();
}

template <typename RanIt, typename CmpT = std::less<>>
void quickSort(RanIt begin, RanIt end, CmpT cmp = CmpT()) {
    quickSort(std::max(static_cast<uint64_t>(1),
                       static_cast<uint64_t>(std::thread::hardware_concurrency() / 2)),
              begin, end, cmp);
}

}  // bench
