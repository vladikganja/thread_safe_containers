#pragma once

#include <lib/common/thread_pools/qs_thread_pool.hpp>

namespace bench {

// Многопоточный квиксорт, который имплементирует внутри логику
// тред-пула, построенного на синхронизированной очереди задач.

namespace impl {

std::optional<ThreadPool<Task, ClassicBQueue>> pool;

template <typename _Ty>
inline _Ty median(_Ty val1, _Ty val2, _Ty val3) {
    if (val1 > val2 && val2 > val3)
        return val2;
    else if (val2 > val1 && val1 > val3)
        return val2;
    return val3;
}

template <typename RanIt, typename cmp_t>
inline RanIt partition(RanIt left, RanIt right, cmp_t cmp) {
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

template <typename RanIt, typename cmp_t = std::less<>>
void task(RanIt begin, RanIt end, cmp_t cmp = cmp_t()) {
    if (begin == end - 1) {
        impl::pool->maxDepthReached();
        return;
    }

    RanIt mid = impl::partition(begin, end, cmp);

    auto&& [lTask, lFut] = createTask(task<RanIt, cmp_t>, begin, mid, cmp);
    auto&& [rTask, rFut] = createTask(task<RanIt, cmp_t>, mid, end, cmp);
    impl::pool->submit(std::move(lTask));
    impl::pool->submit(std::move(rTask));
}

} // impl

template <typename RanIt, typename cmp_t = std::less<>>
void quick_sort(uint64_t threads, RanIt begin, RanIt end, cmp_t cmp = cmp_t()) {
    impl::pool.emplace(threads, std::distance(begin, end));

    auto&& [initTask, fut] = createTask(impl::task<RanIt, cmp_t>, begin, end, cmp);
    impl::pool->submit(std::move(initTask));

    impl::pool->wait();
    impl::pool->stop();
    impl::pool.reset();
}

template <typename RanIt, typename cmp_t = std::less<>>
void quick_sort(RanIt begin, RanIt end, cmp_t cmp = cmp_t()) {
    quick_sort(std::thread::hardware_concurrency(), begin, end, cmp);
}

}  // bench
