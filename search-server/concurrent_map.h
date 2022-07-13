#pragma once
#include <string>
#include <mutex>
#include <map>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count) {}

    Access operator[](const Key& key) {
        auto& bucket = buckets_[key % buckets_.size()];
        return { std::lock_guard<std::mutex>(bucket.mutex_), bucket.map_[key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& bucket : buckets_) {
            std::lock_guard<std::mutex> g(bucket.mutex_);
            for (auto& [key, value] : bucket.map_) {
                result[key] += value;
            }
        }
        return result;
    }

    void Erase(const Key& key) {
        auto& bucket = buckets_[key % buckets_.size()];
        std::lock_guard<std::mutex> g(bucket.mutex_);
        bucket.map_.erase(key);
    }

private:
    struct Bucket {
        std::mutex mutex_;
        std::map<Key, Value> map_;
    };
    std::vector<Bucket> buckets_;
};