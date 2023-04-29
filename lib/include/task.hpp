#pragma once

#include "common.h"

using namespace std::chrono_literals;

enum class TaskSize { Tiny, Small, Medium, Big, Large };

std::map<TaskSize, std::chrono::nanoseconds> dur = {
        {TaskSize::Tiny, 100ns}, {TaskSize::Small, 1us},  {TaskSize::Medium, 100us},
        {TaskSize::Big, 1ms},    {TaskSize::Large, 10ms},
};

template <typename T>
class TaskBase;

template <typename R, typename... Args>
class TaskBase<R(Args...)> {
    std::unique_ptr<void, void (*)(void*)> ptr{nullptr, +[](void*) {}};
    R (*invoke)(void*, Args...) = nullptr;

public:
    TaskBase() = default;
    TaskBase(TaskBase&&) = default;
    TaskBase& operator=(TaskBase&&) = default;

    template <typename F>
    TaskBase(F&& f) {
        auto pf = std::make_unique<F>(std::move(f));
        invoke = +[](void* pf, Args&&... args) -> R {
            F* f = reinterpret_cast<F*>(pf);
            return (*f)(std::forward<Args>(args)...);
        };
        ptr = {pf.release(), [](void* pf) {
                   F* f = reinterpret_cast<F*>(pf);
                   delete f;
               }};
    }

    R operator()(Args&&... args) && {
        R ret = invoke(ptr.get(), std::forward<Args>(args)...);
        clear();
        return std::move(ret);
    }

    void clear() {
        invoke = nullptr;
        ptr.reset();
    }

    explicit operator bool() const {
        return static_cast<bool>(ptr);
    }
};

using Task = TaskBase<int()>;

template <typename Func, typename... Args>
auto createTask(Func func, Args&&... args) {
    std::packaged_task<std::remove_pointer_t<Func>> tsk{func};
    auto fut = tsk.get_future();
    Task t{[ct = std::move(tsk), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        std::apply(
                [ct = std::move(ct)](auto&&... args) mutable {
                    ct(args...);
                },
                std::move(args));
        return 0;
    }};

    return std::make_pair(std::move(t), std::move(fut));
}
