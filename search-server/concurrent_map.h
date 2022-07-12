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

    explicit ConcurrentMap(size_t bucket_count) : mutexes_(bucket_count), maps_(bucket_count), bucket_count_(bucket_count) {}

    Access operator[](const Key& key) {
        size_t index = key % bucket_count_;
        return { std::lock_guard<std::mutex>(mutexes_[index]), maps_[index][key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (size_t i = 0; i < mutexes_.size(); ++i) {
            std::lock_guard<std::mutex> g(mutexes_[i]);
            for (auto& [key, value] : maps_[i]) {
                result[key] += value;
            }
        }
        return result;
    }

    void Erase(const Key& key) {
        size_t index = key % bucket_count_;
        std::lock_guard<std::mutex> g(mutexes_[index]);
        maps_[index].erase(key);
    }

private:
    std::vector<std::mutex> mutexes_;
    std::vector<std::map<Key, Value>> maps_;
    const size_t bucket_count_;
};