#ifndef C_HASHMAP_H
#define C_HASHMAP_H

#include <iostream>
#include <optional>
#include <unordered_map>
#include <zephyr/logging/log.h>

template <typename Key, typename Value>
class CHashMap {
public:
    CHashMap() = default;

    bool Insert(const Key& key, const Value& value) {
        if (!isMainThreadRunning()) {
            if (!map.contains(key) && size >= maxSizeAtStartup) {
                printk("Attempted to insert more than the maximum size of the hashmap post-startup"); // LOG doesn't work well in templates
                k_oops();
                return false;
            }

        }

        if (++size > maxSizeAtStartup) {
            maxSizeAtStartup = size;
        }

        return map.insert(std::make_pair(key, value)).second;
    }

    bool Remove(const Key& key) {
        size--;
        return map.erase(key);
    }

    std::optional<Value> Get(const Key& key) const {
        return map.at(key);
    }

    [[nodiscard]] std::size_t Size() const {
        return size;
    }

    typename std::unordered_map<Key, Value>::iterator begin() {
        return map.begin();
    }

    typename std::unordered_map<Key, Value>::iterator end() {
        return map.end();
    }

private:
    std::unordered_map<Key, Value> map;
    std::size_t size = 0;
    std::size_t maxSizeAtStartup = 0;

    bool isMainThreadRunning() {
        return strncmp(k_thread_name_get(k_current_get()), "main", 4) == 0;
    }
};


#endif //C_HASHMAP_H
