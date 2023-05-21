#include <lib/common/common.h>

template <typename Key, typename Val, typename Hasher = std::hash<Key>>
class concurrent_hash_map {
private:
    static constexpr size_t REHASH_MULTIPLIER = 2;
    static constexpr size_t INIT_BUCKETS = 8;
    static constexpr size_t CHUNKS = 8;
    static constexpr double INIT_MAX_LOAD_FACTOR = 0.618;

    size_t buckets_;
    size_t size_;
    double max_load_factor_;
    double load_factor_;

    using ListKeyValHash = typename std::pair<std::pair<Key, Val>, size_t>;
    using ListIt = typename std::list<ListKeyValHash>::iterator;
    static constexpr auto gethash = [](const ListIt& it) -> size_t {
        return it->second;
    };
    static constexpr auto getpair = [](const ListIt& it) -> std::pair<Key, Val> {
        return it->first;
    };

    std::vector<ListIt> hash_table_;
    mutable std::vector<std::recursive_mutex> locks_;
    std::list<ListKeyValHash> list_;
    Hasher hash_;

    void lock_all() {
        for (auto mut = locks_.begin(); mut != locks_.end(); ++mut) {
            mut->lock();
        }
    }

    void unlock_all() {
        for (auto rmut = locks_.rbegin(); rmut != locks_.rend(); ++rmut) {
            rmut->lock();
        }
    }

    void recalc_load_factor() {
        load_factor_ = static_cast<double>(size_) / static_cast<double>(buckets_);
    }

    void rehash() {
        lock_all();

        buckets_ *= REHASH_MULTIPLIER;
        std::vector<ListIt> new_hash_table(buckets_, list_.end());
        std::swap(hash_table_, new_hash_table);
        ListIt iterator = list_.begin();
        size_t previousHashID = gethash(iterator) % buckets_;
        hash_table_[previousHashID] = iterator;
        ++iterator;
        for (; iterator != list_.end(); ++iterator) {
            size_t currentHashID = gethash(iterator) % buckets_;
            if (currentHashID != previousHashID) {
                hash_table_[currentHashID] = iterator;
            }
            previousHashID = currentHashID;
        }

        recalc_load_factor();

        unlock_all();
    }

public:
    concurrent_hash_map(size_t buckets = INIT_BUCKETS)
            : buckets_(buckets),
              size_(0),
              max_load_factor_(INIT_MAX_LOAD_FACTOR),
              load_factor_(0.0),
              hash_table_(buckets_, list_.end()),
              locks_(CHUNKS) {
    }

    concurrent_hash_map(std::initializer_list<std::pair<Key, Val>> InitList)
            : concurrent_hash_map() {
        insert(InitList.begin(), InitList.end());
    }

    template <typename Iter>
    concurrent_hash_map(Iter first, Iter last)
            : concurrent_hash_map(first, last,
                                  static_cast<size_t>((1.0 / INIT_MAX_LOAD_FACTOR) * 2.0 *
                                                      std::distance(last, first))) {
    }

    template <typename Iter>
    concurrent_hash_map(Iter first, Iter last, size_t buckets): concurrent_hash_map(buckets) {
        insert(first, last);
    }

    ~concurrent_hash_map() {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void insert(const std::pair<Key, Val>& pairKeyVal) {
        const auto& [key, val] = pairKeyVal;
        size_t hashValue = hash_(key);

        std::lock_guard<std::recursive_mutex> guard(locks_[hashValue / buckets_]);

        size_t hashPresentID = hashValue % buckets_;
        size_t hashFutureID = hashValue % (buckets_ * REHASH_MULTIPLIER);
        auto& bucket = hash_table_[hashPresentID];

        if (bucket == list_.end()) {
            list_.push_front({pairKeyVal, hashValue});
            bucket = list_.begin();
        } else {
            auto it = bucket;
            for (; it != list_.end() && hashPresentID == gethash(it) % buckets_; ++it) {
                if (getpair(it).first == key) {
                    return;
                }
            }

            //-------------------------------------------------------------------------------------
            // for the sake of more convenient rehash:
            //
            // present => { HashKey % curSize == HashKey % afterRehashSize } - stays
            //            { HashKey % curSize == HashKey % afterRehashSize } - stays
            //            ...
            //  future => { HashKey % curSize != HashKey % afterRehashSize } - moves
            //            { HashKey % curSize != HashKey % afterRehashSize } - moves
            //            ...
            //-------------------------------------------------------------------------------------

            if (hashPresentID == hashFutureID) {
                bucket = list_.insert(bucket, {pairKeyVal, hashValue});
            } else {
                list_.insert(it, {pairKeyVal, hashValue});
            }
        }

        ++size_;
        recalc_load_factor();

        if (load_factor_ > max_load_factor_) {
            rehash();
        }
    }

    void insert(std::initializer_list<std::pair<Key, Val>> InitList) {
        for (const auto& pairKeyVal : InitList) {
            insert(pairKeyVal);
        }
    }

    template <typename Iter>
    void insert(Iter first, Iter last) {
        for (; first != last; ++first) {
            insert(*first);
        }
    }

    void erase(const Key& key) {
        size_t hashValue = hash_(key);

        std::lock_guard<std::recursive_mutex> guard(locks_[hashValue / buckets_]);

        size_t hashPresentID = hashValue % buckets_;
        auto& bucket = hash_table_[hashPresentID];

        if (bucket != list_.end()) {
            auto it = bucket;
            for (; it != list_.end() && hashPresentID == gethash(it) % buckets_; ++it) {
                if (getpair(it).first == key) {
                    if (it == bucket) {
                        auto next = std::next(it);
                        if (next != list_.end() && gethash(next) % buckets_ == hashPresentID) {
                            bucket = next;
                        } else {
                            bucket = list_.end();
                        }
                    }
                    list_.erase(it);
                    --size_;
                    break;
                }
            }
        }
    }

    void erase(std::initializer_list<Key> InitList) {
        for (const auto& key : InitList) {
            erase(key);
        }
    }

    bool contains(const Key& key) const {
        size_t hashID = hash_(key) % buckets_;
        auto it = hash_table_[hashID];
        if (it != list_.end()) {
            for (; it != list_.end() && hashID == gethash(it) % buckets_; ++it) {
                if (getpair(it).first == key) {
                    return true;
                }
            }
        }
        return false;
    }

    void clear() {
        lock_all();

        list_.clear();
        hash_table_.resize(INIT_BUCKETS);
        for (auto& bucket : hash_table_) {
            bucket = list_.end();
        }

        unlock_all();
    }
};
