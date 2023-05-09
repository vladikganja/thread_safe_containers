// Universal template-based muiltithread queue

#pragma once

#include <lib/common/task.hpp>

enum class Base { Array = 0, List = 1, Deque = 2 };

enum class Bounded { Yes = 3, No = 4 };

enum class Priority { Yes = 5, No = 6 };

enum class Lifo { Strong = 7, PerThread = 8, BestEffort = 9, No = 10 };

enum class Contention { Waitfree = 11, Lockfree = 12, Blocking = 13 };

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
    mask |= (1 << static_cast<uint64_t>(Contention::Blocking));
    return mask;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// MASK-TRAITS
///////////////////////////////////////////////////////////////////////////////////////////////////

template <uint64_t Val, uint64_t Mask>
struct is {
    enum { value = static_cast<bool>((1 << Val) & Mask) };
};

template <uint64_t Mask>
struct isArray {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Base::Array)) & Mask) };
};

template <uint64_t Mask>
struct isBounded {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Bounded::Yes)) & Mask) };
};

template <uint64_t Mask>
struct isUnbounded {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Bounded::No)) & Mask) };
};

template <uint64_t Mask>
struct isPriority {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Priority::Yes)) & Mask) };
};

template <uint64_t Mask>
struct isWaitFree {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Contention::Waitfree)) & Mask) };
};

template <uint64_t Mask>
struct isLockFree {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Contention::Lockfree)) & Mask) };
};

template <uint64_t Mask>
struct isBlocking {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Contention::Blocking)) & Mask) };
};

} // namespace

//! Base::Deque, Bounded::No, Priority::No, Lifo::Strong, Contention::Blocking
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// ARRAY-CORE
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ValT, typename CmpT = std::less<>>
class BinHeap {
private:
    std::vector<ValT> array_;

    void sift_down(size_t i) {
        CmpT cmp = CmpT();
        while (true) {
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;
            size_t largest = i;

            if (left < array_.size() && cmp(array_[i], array_[left])) {
                largest = left;
            }
            if (right < array_.size() && cmp(array_[largest], array_[right])) {
                largest = right;
            }

            if (largest != i) {
                std::swap(array_[i], array_[largest]);
                i = largest;
            } else {
                break;
            }
        }
    }

    void sift_up(size_t i) {
        CmpT cmp = CmpT();
        while (i > 0) {
            size_t parent = (i - 1) / 2;

            if (cmp(array_[parent], array_[i])) {
                std::swap(array_[parent], array_[i]);
                i = parent;
            } else {
                break;
            }
        }
    }

public:
    BinHeap() {
    }

    template <typename RanIt>
    BinHeap(RanIt begin, RanIt end): array_(begin, end) {
        for (size_t i = array_.size() / 2 - 1; i != SIZE_MAX; i--) {
            sift_down(i);
        }
    }

    BinHeap(const BinHeap& other) {
        array_ = other.array_;
    }

    BinHeap(BinHeap&& other) noexcept {
        array_ = std::move(other.array_);
    }

    BinHeap& operator=(const BinHeap& other) {
        array_ = other.array_;
        return *this;
    }

    BinHeap& operator=(BinHeap&& other) noexcept {
        array_ = std::move(other.array_);
        return *this;
    }

    virtual ~BinHeap() {
    }

public:
    void h_insert(const ValT& val) {
        array_.push_back(val);
        sift_up(array_.size() - 1);
    }

    void h_erase_by_index(size_t index) {
        assert(index < array_.size());
        std::swap(array_[array_.size() - 1], array_[index]);
        array_.pop_back();
        sift_down(index);
        sift_up(index);
    }

    void h_erase_by_value(const ValT& val) {
        CmpT cmp = CmpT();
        auto it = std::find(array_.begin(), array_.end(), val);
        if (it != array_.end()) {
            h_erase_by_index(std::distance(it, array_.begin()));
        }
    }

    void h_erase_top() {
        h_erase_by_index(0);
    }

    void h_erase_bottom() {
        CmpT cmp = CmpT();
        auto last_element = array_.begin() + (array_.size() / 2);
        for (auto it = array_.begin() + (array_.size() / 2); it != array_.end(); ++it) {
            if (cmp(*it, *last_element)) {
                last_element = it;
            }
        }
        erase_by_index(std::distance(last_element, array_.begin()));
    }

    ValT h_top() const {
        assert(array_.size() > 0);
        return array_[0];
    }

    ValT h_bottom() const {
        assert(array_.size() > 0);
        CmpT cmp = CmpT();
        auto last_element = array_.begin() + (array_.size() / 2);
        for (auto it = array_.begin() + (array_.size() / 2); it != array_.end(); ++it) {
            if (cmp(*it, *last_element)) {
                last_element = it;
            }
        }
        return *last_element;
    }

    void h_print() const {
        for (auto& el : array_) {
            std::cout << el << ' ';
        }
        std::cout << std::endl;
    }

    size_t size() const {
        return array_.size();
    }

    bool empty() const {
        return array_.empty();
    }
};

template <typename ValT>
class PriorityQueue : public BinHeap<ValT> {
public:
    PriorityQueue() {
    }

    template <typename RanIt>
    PriorityQueue(RanIt begin, RanIt end): BinHeap<ValT>(begin, end) {
    }

    ~PriorityQueue() override {
    }

    void push(const ValT& t) {
        h_insert(t);
    }

    // void pop() {
    //     h_erase_bottom();
    // }
};

template <typename TaskT, uint64_t Mask, typename Enable = void>
class ArrayCore {};
// VECTOR-IMPL          // HEAP-IMPL
// 34****12             // 4321****
//  |    |              // |  |
//  b    f              // f  b

template <typename TaskT, uint64_t Mask>
class ArrayCore<TaskT, Mask, typename std::enable_if_t<isBlocking<Mask>::value>> {
public:
    using ArrayT =
            std::conditional_t<isPriority<Mask>::value, PriorityQueue<TaskT>, std::vector<TaskT>>;

    ArrayT buffer_;
    ArrayCore(uint64_t limit) : buffer_(limit) {
    }

    uint64_t size_{};
    mutable std::mutex mut_;
    std::condition_variable condProd_;
    std::condition_variable condCons_;
    uint64_t dequeuePos_{};
    uint64_t enqueuePos_{};
    bool full_ = false;
    bool empty_ = true;
    bool done_ = false;
};

template <typename TaskT>
struct Cell {
    std::atomic<uint64_t> sequence;
    TaskT task;
};

template <typename TaskT, uint64_t Mask>
class ArrayCore<TaskT, Mask, typename std::enable_if_t<isLockFree<Mask>::value>> {
public:
    std::vector<TaskT> buffer_;

    ArrayCore(uint64_t limit): buffer_(limit) {
    }

    uint64_t size_{};
    uint64_t bufMask_{};
    std::atomic<uint64_t> dequeuePos_{};
    std::atomic<uint64_t> enqueuePos_{};
};

// ARRAY-BASED BOUNDED/UNBOUNDED PRIOR/NOPRIOR BLOCKING/LOCK-FREE
template <typename TaskT, uint64_t SpecMask>
class ArrayBehave {
private:
    // Priority queue can be only blocking
    static_assert(!(isPriority<SpecMask>::value && isWaitFree<SpecMask>::value));
    static_assert(!(isPriority<SpecMask>::value && isLockFree<SpecMask>::value));

    ArrayCore<typename std::conditional_t<isLockFree<SpecMask>::value, Cell<TaskT>, TaskT>,
              SpecMask>
            core_;

    void lfEnqueue(TaskT&& task) {
        INFO("lfenqueue");
        Cell<TaskT>* cell;
        uint64_t pos;
        bool res = false;

        while (!res) {
            // fetch the current Position where to enqueue the item
            pos = core_.enqueuePos_.load();
            cell = &core_.buffer_[pos & core_.bufMask_];
            auto seq = cell->sequence.load();
            auto diff = static_cast<int>(seq) - static_cast<int>(pos);

            // queue is full we cannot enqueue and just continue
            // another option: queue moved forward all way round
            if (diff < 0) {
                continue;
            }

            // If its Sequence wasn't touched by other producers
            // check if we can increment the enqueue Position
            if (diff == 0) {
                INFO("wtf");
                res = core_.enqueuePos_.compare_exchange_weak(pos, pos + 1);
            }
        }

        // write the item we want to enqueue and bump Sequence
        cell->task = std::move(task);
        cell->sequence.store(pos + 1);
    }

    bool lfDequeue(TaskT& task) {
        Cell<TaskT>* cell;
        uint64_t pos;
        bool res = false;

        while (!res) {
            // fetch the current Position from where we can dequeue an item
            pos = core_.dequeuePos_.load();
            cell = &core_.buffer_[pos & core_.bufMask_];
            auto seq = cell->sequence.load();
            auto diff = static_cast<int>(seq) - static_cast<int>(pos + 1);

            // probably the queue is empty, then return false
            if (diff < 0) {
                return false;
            }

            // Check if its Sequence was changed by a producer and wasn't changed by
            // other consumers and if we can increment the dequeue Position
            if (diff == 0) {
                res = core_.dequeuePos_.compare_exchange_weak(pos, pos + 1);
            }
        }

        // read the item and update for the next round of the buffer
        task = std::move(cell->task);
        cell->sequence.store(pos + core_.bufMask_ + 1);
        return true;
    }

    // mess because of my brain is bleeding after 20 hours of sfinae
    void blkUnbounded(TaskT&& task) {
        if (core_.size_ == core_.buffer_.size()) {
            core_.buffer_.resize(core_.size_ * 2);
        }
        core_.buffer_[core_.enqueuePos_] = std::move(task);
        core_.enqueuePos_++;
    }

    void blkBounded(TaskT&& task) {
        if constexpr (isPriority<SpecMask>::value) {
            core_.buffer_.push(std::move(task));
        } else {
            core_.buffer_[core_.enqueuePos_] = std::move(task);
            core_.enqueuePos_ = (core_.enqueuePos_ + 1) % core_.buffer_.size();
        }

        core_.size_++;
        if (core_.size_ == core_.buffer_.size()) {
            core_.full_ = true;
        }
        core_.empty_ = false;
    }

    void blkEnqueue(TaskT&& task) {
        std::unique_lock<std::mutex> guard{core_.mut_};

        if constexpr (isBounded<SpecMask>::value) {
            core_.condProd_.wait(guard, [this] {
                return !core_.full_;
            });

            blkBounded(std::move(task));

            guard.unlock();
            core_.condCons_.notify_one();
        } else if (isUnbounded<SpecMask>::value) {
            blkUnbounded(std::move(task));
        }
    }

    bool blkDequeue(TaskT& task) {
        std::unique_lock<std::mutex> guard{core_.mut_};
        // PRIORITY !!! TODO
        if constexpr (isBounded<SpecMask>::value) {
            core_.condCons_.wait(guard, [this] {
                return !core_.empty_ || core_.done_;
            });

            if (core_.empty_) {
                return false;
            }

            task = std::move(core_.buffer_[core_.dequeuePos_]);
            core_.dequeuePos_ = (core_.dequeuePos_ + 1) % core_.buffer_.size();

            core_.size_--;
            if (core_.size_ == 0) {
                core_.empty_ = true;
            }
            core_.full_ = false;

            guard.unlock();
            core_.condProd_.notify_one();
        } else if (isUnbounded<SpecMask>::value) {

        }

        return true;
    }

    void doneWakeUp() {
        std::unique_lock<std::mutex> guard{core_.mut_};
        core_.done_ = true;
        guard.unlock();
        core_.condCons_.notify_all();
    }

    bool emptyAndDone() const {
        std::unique_lock<std::mutex> guard{core_.mut_};
        return core_.empty_ && core_.done_;
    }

public:

    ArrayBehave(uint64_t limit) : core_(limit) {
        if constexpr (isLockFree<SpecMask>::value) {
            for (uint64_t i = 0; i < limit; i++) {
                core_.buffer_[i].sequence.store(i);
            }

            core_.enqueuePos_.store(0);
            core_.dequeuePos_.store(0);

            core_.bufMask_ = limit - 1;
        }
    }

    void enqueue(TaskT&& task) {
        if constexpr (isLockFree<SpecMask>::value) {
            lfEnqueue(std::move(task));
        } else {
            blkEnqueue(std::move(task));
        }
    }

    bool dequeue(TaskT& task) {
        if constexpr (isLockFree<SpecMask>::value) {
            return lfDequeue(task);
        } else {
            return blkDequeue(task);
        }
    }

    bool empty() const {
        if constexpr (isLockFree<SpecMask>::value) {
            return core_.enqueuePos_.load() == core_.dequeuePos_.load();
        } else {
            return emptyAndDone();
        }
    }

    void wakeUp() {
        if constexpr (isBlocking<SpecMask>::value) {
            doneWakeUp();
        }
    }
};

template <typename TaskT, uint64_t SpecMask>
class uniQueue<TaskT, SpecMask, typename std::enable_if<isArray<SpecMask>::value>::type> {
private:
    static constexpr uint64_t INIT_SIZE = 32;
    ArrayBehave<TaskT, SpecMask> behaveCore_;

public:
    static constexpr uint64_t spec = SpecMask;

    template <uint64_t Mask = SpecMask>
    explicit uniQueue(uint64_t limit, typename std::enable_if_t<isBounded<Mask>::value>* = nullptr)
            : behaveCore_(limit) {
        INFO("array! limit: ", limit);

    }

    template <uint64_t Mask = SpecMask>
    uniQueue(typename std::enable_if_t<!isBounded<Mask>::value>* = nullptr)
            : behaveCore_(INIT_SIZE) {
        INFO("array! no limit");

    }

    void enqueue(TaskT&& task) {
        behaveCore_.enqueue(std::move(task));
    }

    bool dequeue(TaskT& task) {
        return behaveCore_.dequeue(task);
    }

    bool empty() const {
        return behaveCore_.empty();
    }

    void wakeUp() {
        behaveCore_.wakeUp();
    }
};
