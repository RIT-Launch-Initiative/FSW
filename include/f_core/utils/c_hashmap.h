#ifndef C_HASHMAP_H
#define C_HASHMAP_H

#include <optional>
#include <unordered_map>
#include <zephyr/logging/log.h>

/**
 * Wrapper around C++ unordered_map that blocks inserting new keys during RTOS runtime (threads)
 * Main thread is the only thread allowed to allocate to the map, so the function is expected
 * to only be called when tenants are initializing
 *
 * @tparam KeyType Type for the key
 * @tparam ValueType Type for the value
 */
template <typename KeyType, typename ValueType>
class CHashMap {
public:
    CHashMap() = default;

    bool Insert(const KeyType& key, const ValueType& value) {
        if (!isMainThreadRunning()) {
            if (!map.contains(key) && size > maxSizeAtStartup) {
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

    bool Set(const KeyType& key, const ValueType& value) {
        if (map.contains(key)) {
            map[key] = value;
            return true;
        }

        return false;
    }

    bool Remove(const KeyType& key) {
        size--;
        return map.erase(key);
    }

    std::optional<ValueType> Get(const KeyType& key) const {
        return map.at(key);
    }

    bool Contains(KeyType key) const {
        return map.contains(key);
    }

    [[nodiscard]] std::size_t Size() const {
        return size;
    }

    typename std::unordered_map<KeyType, ValueType>::iterator begin() {
        return map.begin();
    }

    typename std::unordered_map<KeyType, ValueType>::iterator end() {
        return map.end();
    }

    ValueType& operator[](const KeyType& key) {
        if (!map.contains(key)) {
            printk("Attempted to access a key that does not exist in the hashmap"); // LOG doesn't work well in templates
            k_oops();
        }

        return map[key];
    }

private:
    std::unordered_map<KeyType, ValueType> map;
    std::size_t size = 0;
    std::size_t maxSizeAtStartup = 0;

    bool isMainThreadRunning() {
        return strncmp(k_thread_name_get(k_current_get()), "main", 4) == 0;
    }
};


#endif //C_HASHMAP_H
