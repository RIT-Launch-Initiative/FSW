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

    CHashMap(std::initializer_list<std::pair<const KeyType, ValueType>> initList) {
        for (const auto& pair : initList) {
            Insert(pair.first, pair.second);
        }
    }

    bool Insert(const KeyType& key, const ValueType& value) {
        if (!isMainThreadCurrent()) {
            if (!map.contains(key) && size > maxSizeReachedAtStartup) {
                printk("Attempted to insert more than the maximum size of the hashmap post-startup\n"); // LOG doesn't work well in templates
// Only fatal in debug mode. Unlikely to occur in an actual flight, and in the off-chance it does we shouldn't fatal the entire system over it
#ifdef CONFIG_DEBUG
                k_oops();
#endif

                return false;
            }

        }

        if (++size > maxSizeReachedAtStartup) {
            maxSizeReachedAtStartup = size;
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
        if (!map.contains(key)) {
            return std::nullopt;
        }

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

    // Please don't add a [] operator. It can be unintentionally abused
    // Can unintentionally assign new keys to the map, and handling a failing contains check
    // gets tricky when you can't return optional. Learn from my mistakes :)
    // - Aaron

private:
    std::unordered_map<KeyType, ValueType> map;
    std::size_t size = 0;
    std::size_t maxSizeReachedAtStartup = 0;

    bool isMainThreadCurrent() {
        return strncmp(k_thread_name_get(k_current_get()), "main", 4) == 0;
    }
};


#endif //C_HASHMAP_H
