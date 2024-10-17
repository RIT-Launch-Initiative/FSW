#pragma once
#include <array>

template <typename ValueT, std::size_t Length> class CircularBuffer {
  public:
    using value_type = ValueT;
    static constexpr std::size_t size_ = Length;
    constexpr CircularBuffer(const value_type &initial_value) { fill(initial_value); }

    constexpr void fill(const value_type &value) { underlying.fill(value); }

    constexpr void add_sample(const value_type &value) {
        underlying[oldest_index] = value;
        oldest_index++;
        oldest_index %= size_;
    }

    constexpr std::size_t size() const { return size_; }

    constexpr value_type &oldest_sample() { return underlying[oldest_index]; }
    constexpr value_type &newest_sample() {
        std::size_t newset_index = oldest_index - 1;
        if (newset_index < 0) {
            newset_index += size_;
        }
        return underlying[newset_index];
    }

    // index of 0 is the oldest sample.
    // index of size()-1 is the newest sample
    // values > size() will wrap around
    constexpr value_type &operator[](std::size_t index) {
        std::size_t real_index = oldest_index + index;
        real_index %= size_;
        return underlying[real_index];
    }

  private:
    std::size_t oldest_index = 0;
    std::array<value_type, size_> underlying;
};
