// Universal template-based muiltithread queue

#pragma once

#include <lib/common/task.hpp>

enum class Base { Array = 0, List = 1, Deque = 2 };

enum class Bounded { Yes = 3, No = 4 };

enum class Priority { Yes = 5, No = 6 };

enum class Lifo { Strong = 7, PerThread = 8, BestEffort = 9, No = 10 };

enum class Free { Wait = 11, Lock = 12, Blocking = 13 };

namespace {

template <typename T, T Begin, class Func, T... Is>
constexpr void staticForImpl(Func&& f, std::integer_sequence<T, Is...>) noexcept {
    (f(std::integral_constant<T, Begin + Is>{}), ...);
}

template <typename T, T Begin, T End, class Func>
constexpr void staticFor(Func&& f) noexcept {
    staticForImpl<T, Begin>(std::forward<Func>(f), std::make_integer_sequence<T, End - Begin>{});
}

template <class Tuple>
constexpr std::size_t tupleSize(const Tuple&) noexcept {
    return std::tuple_size<Tuple>::value;
}

constexpr uint64_t defaultQSpec() noexcept {
    uint64_t mask = 0;
    mask |= (1 << static_cast<uint64_t>(Base::Deque));
    mask |= (1 << static_cast<uint64_t>(Bounded::No));
    mask |= (1 << static_cast<uint64_t>(Priority::No));
    mask |= (1 << static_cast<uint64_t>(Lifo::Strong));
    mask |= (1 << static_cast<uint64_t>(Free::Blocking));
    return mask;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <uint64_t Mask>
struct isArray {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Base::Array)) & Mask) };
};

template <uint64_t Mask>
struct isBounded {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Bounded::Yes)) & Mask) };
};

template <uint64_t Mask>
struct isPriority {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Priority::Yes)) & Mask) };
};

} // namespace

//! Base::Deque, Bounded::No, Priority::No, Lifo::Strong, Free::Blocking
template <typename... Args>
constexpr uint64_t uniQSpec(Args&&... args) noexcept {
    auto tuple = std::make_tuple(std::forward<Args>(args)...);

    uint64_t mask = 0;

    staticFor<std::size_t, 0, tupleSize(tuple)>([&](auto i) mutable {
        mask |= (1 << static_cast<uint64_t>(std::get<i>(tuple)));
    });

    return mask;
}

template <typename TaskT, uint64_t SpecMask = defaultQSpec(), typename Enable = void>
class uniQueue {
private:

public:
    //! this is not an array implementation, so priority option is impossible
    static_assert(!isPriority<SpecMask>::value);

    uniQueue() {
        INFO("Not array");
    }
};

template <typename TaskT, uint64_t SpecMask, typename Enable = void>
class ArrayCore {
private:
    std::vector<TaskT> buffer_;

public:
    void core() {
        INFO("not priority");
    }
};

// template <typename TaskT, uint64_t SpecMask>
// class ArrayCore<TaskT, SpecMask, typename std::enable_if<isPriority<SpecMask>::value>::type> {
// private:
//     std::vector<TaskT> buffer_;

// public:
//     ArrayCore() {
//     }

//     void core() {
//         INFO("priority");
//     }
// };

template <typename TaskT, uint64_t SpecMask>
class uniQueue<TaskT, SpecMask, typename std::enable_if<isArray<SpecMask>::value>::type> {
private:
    ArrayCore<TaskT, SpecMask> core_;

public:
    template <typename std::enable_if_t<isBounded<SpecMask>::value>* = nullptr>
    uniQueue(uint64_t limit) {
        INFO("array! limit: ", limit);
        core_.core();
    }

    uniQueue() {
        INFO("array!");
        core_.core();
    }

    void enqueue() {

    }

    void dequeue() {

    }
};
