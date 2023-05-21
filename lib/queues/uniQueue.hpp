// Universal template-based muiltithread queue

#pragma once

#include <lib/common/task.hpp>
#include <lib/common/hazard.h>

enum class Base { Array = 0, List = 1 };

enum class Bounded { Yes = 2, No = 3 };

enum class Priority { Yes = 4, No = 5 };

enum class Contention { Lockfree = 6, Blocking = 7 };

constexpr uint64_t ARRAY = 1;
constexpr uint64_t LIST = 2;
constexpr uint64_t BOUNDED = 4;
constexpr uint64_t UNBOUNDED = 8;
constexpr uint64_t PRIOR = 16;
constexpr uint64_t NOTPRIOR = 32;
constexpr uint64_t LOCKFREE = 64;
constexpr uint64_t BLOCKING = 128;

// IMPLEMENTED:
// list lockfree unbounded
// array blocking bounded
// array lock-free bounded
// array blocking bounded priority

namespace {

namespace smr {

static std::array<hp::ThreadLocalHazardManager*, 
    THREADS_COUNT> globalMaster;
thread_local hp::ThreadLocalHazardManager 
    myMaster(globalMaster);

} // smr

namespace inner {

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
    mask |= (1 << static_cast<uint64_t>(Base::List));
    mask |= (1 << static_cast<uint64_t>(Bounded::No));
    mask |= (1 << static_cast<uint64_t>(Priority::No));
    mask |= (1 << static_cast<uint64_t>(Contention::Lockfree));
    return mask;
}

} // inner

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
struct isLockFree {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Contention::Lockfree)) & Mask) };
};

template <uint64_t Mask>
struct isBlocking {
    enum { value = static_cast<bool>((1 << static_cast<uint64_t>(Contention::Blocking)) & Mask) };
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace

//! Base::List, Bounded::No, Priority::No, Contention::Lockfree
template <typename... Args>
constexpr uint64_t uniQSpec(Args&&... args) noexcept {
    auto tuple = std::make_tuple(std::forward<Args>(args)...);

    uint64_t mask = 0;

    inner::staticFor<std::size_t, 0, inner::tupleSize(tuple)>([&](auto i) mutable {
        mask |= (1 << static_cast<uint64_t>(std::get<i>(tuple)));
    });

    return mask;
}

template <typename TaskT, uint64_t SpecMask>
class uniQueue {};

// Classic Michael-Scott Queue
template <typename TaskT>
class uniQueue<TaskT, LIST | UNBOUNDED | LOCKFREE | NOTPRIOR> {
private:
    struct Node {
        std::atomic<Node*> next = nullptr;
        TaskT task;

        Node() {}
        Node(TaskT&& task) : task(std::move(task)) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

// Пример неправильного написания lock-free на C++ ////////////////////////////
/*
    void ABAenqueue(TaskT&& task) {
        std::atomic<Node*> newTail = new Node(std::move(task));

        while (true) {
            Node* curTail = tail_.load();
            if (curTail->next.compare_exchange_strong(nullptr, newTail)) {
                tail_.compare_exchange_strong(curTail, newTail);
                return;
            } else {
                tail_.compare_exchange_strong(curTail, curTail->next.load());
            }
        }
    }
*/
///////////////////////////////////////////////////////////////////////////////

public:
    uniQueue() {
        Node* sentinel = new Node;
        head_.store(sentinel, std::memory_order_release);
        tail_.store(sentinel, std::memory_order_release);
    }

    ~uniQueue() {
        // Деструктор вызывается главным потоком
        Node* cur = head_;
        do {
            Node* toDel = cur;
            cur = cur->next;
            delete toDel;
        } while (cur->next);
    }

    void enqueue(TaskT&& task) {
        Node* newTail = new Node(std::move(task));

        while (true) {
            Node* curTail = tail_.load(std::memory_order_relaxed);

            // объявление указателя как hazard. HP – thread-private массив
            smr::myMaster.hptrs[0] = curTail;

            // обязательно проверяем, что tail_ не изменился
            if (curTail != tail_.load(std::memory_order_acquire)) {
                continue;
            }

            Node* next = curTail->next.load();

            if (curTail != tail_.load(std::memory_order_acquire)) {
                continue;
            }

            if (next != nullptr) {
                tail_.compare_exchange_strong(curTail, next, std::memory_order_release);
                continue; // ?
            }

            Node* nextNullptr = nullptr;
            if (curTail->next.compare_exchange_strong(nextNullptr, newTail,
                                                      std::memory_order_release)) {
                tail_.compare_exchange_strong(curTail, newTail, std::memory_order_acq_rel);
                smr::myMaster.hptrs[0] = nullptr; // обнуляем hazard pointer
                break;
            }
        }
    }

    bool dequeue(TaskT& task) {
        while (true) {
            Node* curHead = head_.load(std::memory_order_relaxed);
            smr::myMaster.hptrs[0] = curHead;

            if (curHead != head_.load(std::memory_order_acquire)) {
                continue;
            }

            Node* curTail = tail_.load(std::memory_order_relaxed);
            Node* next = curHead->next.load(std::memory_order_acquire);

            smr::myMaster.hptrs[1] = next;

            if (curHead != head_.load(std::memory_order_relaxed)) {
                continue;
            }

            if (next == nullptr) {
                smr::myMaster.hptrs[0] = nullptr;
                return false;
            }

            if (curHead == curTail) {
                tail_.compare_exchange_strong(curTail, next, std::memory_order_release);
                continue;
            }

            task = std::move(next->task);

            // Есть ненулевая вероятность, что T1 запомнит в регистрах curHead и Next, при этом не успеет
            // сделать CAS и будет вытеснен планировщиком -> потенциальная ABA.
            // Но этого не произойдёт, так как, благодаря системе hazard-указателей, 
            // curHead и next не могли быть отданы аллокатору из других потоков, следовательно, 
            // не могли быть повторно аллоцированы.
            if (head_.compare_exchange_strong(curHead, next, std::memory_order_release)) {
                smr::myMaster.hptrs[0] = nullptr;
                smr::myMaster.hptrs[1] = nullptr;

                smr::myMaster.RetireNode(curHead);
                break;
            }
        }

        return true;
    }

    bool empty() const {
        return head_.load() == tail_.load();
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

template <typename TaskT>
class uniQueue<TaskT, ARRAY | BLOCKING | NOTPRIOR | BOUNDED> {
private:
    uint64_t size_{};

    mutable std::mutex mut_;
    std::condition_variable condProd_;
    std::condition_variable condCons_;

    std::vector<TaskT> buffer_;

    uint64_t dequeuePos_{};
    uint64_t enqueuePos_{};

    bool full_ = false;
    bool empty_ = true;
    bool done_ = false;

    // 34****12
    //  |    |
    //  b    f

public:
    BlockingBoundedQueue(): BlockingBoundedQueue(32){};
    BlockingBoundedQueue(uint64_t size): buffer_(size) {
    }

    // Some producer sends the data to the queue
    // Then condProd_ waits while buffer is full or skips waiting if it's not full
    // Then it pushes data to the end of the queue (if enqueuePos_ overtakes frontIdx then the queue
    // is full)
    void enqueue(TaskT&& task) {
        std::unique_lock<std::mutex> guard{mut_};

        condProd_.wait(guard, [this] {
            return !full_;
        });

        buffer_[enqueuePos_] = std::move(task);
        enqueuePos_ = (enqueuePos_ + 1) % buffer_.size();

        size_++;
        if (size_ == buffer_.size()) {
            full_ = true;
        }
        empty_ = false;

        guard.unlock();
        condCons_.notify_one();
    }

    bool dequeue(TaskT& task) {
        std::unique_lock<std::mutex> guard{mut_};

        condCons_.wait(guard, [this] {
            return !empty_ || done_;
        });

        if (empty_) {
            return false;
        }

        task = std::move(buffer_[dequeuePos_]);
        dequeuePos_ = (dequeuePos_ + 1) % buffer_.size();

        size_--;
        if (size_ == 0) {
            empty_ = true;
        }
        full_ = false;

        guard.unlock();
        condProd_.notify_one();
        return true;
    }

    void wakeUp() {
        std::unique_lock<std::mutex> guard{mut_};
        done_ = true;
        guard.unlock();
        condCons_.notify_all();
    }

    bool empty() const {
        std::unique_lock<std::mutex> guard{mut_};
        return empty_ && done_;
    }
};

template <typename TaskT>
class uniQueue<TaskT, ARRAY | LOCKFREE | NOTPRIOR | BOUNDED> {
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

template <typename TaskT>
class uniQueue<TaskT, ARRAY | BLOCKING | NOTPRIOR | UNBOUNDED> {
private:
    mutable std::mutex mut_;
    std::queue<TaskT, std::deque<TaskT>> queue_;

public:
    void enqueue(TaskT&& task) {
        std::lock_guard<std::mutex> guard{mut_};
        queue_.push(std::move(task));
    }

    bool dequeue(TaskT& task) {
        std::unique_lock<std::mutex> guard{mut_};

        if (queue_.empty()) {
            return false;
        }

        task = std::move(queue_.front());
        queue_.pop();

        return true;
    }

    bool empty() const {
        return queue_.empty();
    }
};
