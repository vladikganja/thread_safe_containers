#pragma once

#include <lib/common/common.h>

/*

std::unique_pointer<void, deleter> is an example of type erasure in C++. Type erasure is a technique
that allows us to write generic code that can work with objects of different types without knowing
the exact type at compile time. It is often used to implement polymorphism and to hide
implementation details.

In this case, we use void as the type parameter for std::unique_pointer to indicate that we don't
care about the exact type of the object being managed by the unique pointer. This allows us to write
code that can work with any type of object, as long as we provide a suitable deleter.

The deleter is a function object that is responsible for deleting the object when the unique pointer
goes out of scope. By using a type-erased unique pointer with a custom deleter, we can write code
that can work with any type of object, while still ensuring that the object is properly deleted.

*/

namespace {

template <typename T>
class ScaryTaskBase;

template <typename RetT, typename... Args>
class ScaryTaskBase<RetT(Args...)> {
private:
    std::unique_ptr<void, void (*)(void*)> ptr_{nullptr, [](void*) {}};
    RetT (*invoke_)(void*, Args...) = nullptr;

public:
    ScaryTaskBase() = default;
    ScaryTaskBase(ScaryTaskBase&&) = default;
    ScaryTaskBase& operator=(ScaryTaskBase&&) = default;

    template <typename FuncT>
    ScaryTaskBase(FuncT&& func) {
        auto fPtr = std::make_unique<FuncT>(std::move(func));
        invoke_ = [](void* fPtr, Args&&... args) -> RetT {
            FuncT* f = reinterpret_cast<FuncT*>(fPtr);
            return (*f)(std::forward<Args>(args)...);
        };
        ptr_ = {fPtr.release(), [](void* fPtr) {
                    FuncT* del = reinterpret_cast<FuncT*>(fPtr);
                    delete del;
                }};
    }

    RetT operator()(Args&&... args) && {
        RetT ret = invoke_(ptr_.get(), std::forward<Args>(args)...);
        clear();
        return std::move(ret);
    }

    void clear() {
        invoke_ = nullptr;
        ptr_.reset();
    }

    explicit operator bool() const {
        return static_cast<bool>(ptr_);
    }
};

} // namespace

using Task = ScaryTaskBase<int()>;

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
