#pragma once

#include <array>

template <typename ValueT, std::size_t Length>
class CCircularBuffer {
  public:
    using value_type = ValueT;
    static constexpr std::size_t size_ = Length;
    constexpr CCircularBuffer(const value_type &initial_value) { Fill(initial_value); }

    constexpr void Fill(const value_type &value) { underlying.fill(value); }

    constexpr void AddSample(const value_type &value) {
        underlying[oldest_index] = value;
        oldest_index++;
        oldest_index %= size_;
    }

    constexpr std::size_t Size() const { return size_; }

    constexpr value_type &OldestSample() { return underlying[oldest_index]; }
    constexpr value_type &NewestSample() {
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
