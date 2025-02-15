#ifndef C_HASHMAP_H
#define C_HASHMAP_H

#include <iostream>
#include <optional>

template <typename Key, typename Value, std::size_t Capacity>
class CHashMap {
public:
    bool Insert(const Key& key, const Value& value) {
        const std::size_t index = hash(key);
        for (std::size_t i = 0; i < Capacity; ++i) {
            std::size_t probeIndex = (index + i) % Capacity;
            if (!data[probeIndex].occupied || data[probeIndex].key == key) {
                data[probeIndex] = {key, value, true};
                return true;
            }
        }
        return false;
    }

    bool Remove(const Key& key) {
        const std::size_t index = hash(key);
        for (std::size_t i = 0; i < Capacity; ++i) {
            std::size_t probeIndex = (index + i) % Capacity;
            if (!data[probeIndex].occupied) break;

            if (data[probeIndex].key == key) {
                data[probeIndex].occupied = false;
                return true;
            }
        }
        return false;
    }

    std::optional<Value> Get(const Key& key) const {
        const std::size_t index = hash(key);
        for (std::size_t i = 0; i < Capacity; ++i) {
            std::size_t probeIndex = (index + i) % Capacity;
            if (!data[probeIndex].occupied) break;  // Stop if empty slot found
            if (data[probeIndex].key == key) return data[probeIndex].value;
        }
        return std::nullopt;
    }
private:
    struct Entry {
        Key key;
        Value value;
        bool occupied = false;
    };

    Entry data[Capacity] = {};

    std::size_t hash(const Key& key) const {
        return std::hash<Key>{}(key) % Capacity;
    }
};


#endif //C_HASHMAP_H
