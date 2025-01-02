#include "f_core/util/circular_buffer.hpp"

#include <array>
#include <cstddef>

template <typename T, std::size_t len>
class CRollingSum {
  public:
    static_assert(len > 0, "What is the sum of 0 elements? You probably don't want this (also it will break)");

    using value_type = T;
    static constexpr std::size_t size_ = len;

    constexpr CRollingSum(T start) : buf(start) { fill(start); }

    constexpr std::size_t size() const { return size_; }

    constexpr void fill(const value_type &start) {
        buf.Fill(start);
        total = buf.OldestSample();
        for (std::size_t i = 1; i < buf.size(); i++) {
            total = total + buf[1];
        }
    }

    constexpr void feed(const value_type &new_value) {
        value_type oldest = buf.OldestSample();
        total = total - oldest;
        total = total + new_value;
        buf.AddSample(new_value);
    }
    constexpr value_type sum() const { return total; }

  private:
    value_type total;
    CCircularBuffer<value_type, size_> buf;
};

template <typename T, std::size_t len>
class CMovingAverage {
  public:
    using value_type = T;
    static constexpr std::size_t length = len;

    constexpr CMovingAverage(value_type start) : summer(start) {}

    constexpr void feed(value_type value) { summer.feed(value); }
    constexpr value_type avg() { return summer.sum() / (value_type) len; }
    constexpr void fill(const T &start) { summer.fill(start); }

  private:
    CRollingSum<value_type, len> summer;
};

template <typename T>
struct LinearFitSample {
    using SampleT = T;
    LinearFitSample() : LinearFitSample(0, 0) {}
    LinearFitSample(T x, T y) : x(x), y(y), xx(x * x), xy(x * y) {}
    LinearFitSample(T x, T y, T xx, T xy) : x(x), y(y), xx(xx), xy(xy) {}

    LinearFitSample operator+(const LinearFitSample &o) const {
        return LinearFitSample{x + o.x, y + o.y, xx + o.xx, xy + o.xy};
    }
    LinearFitSample operator-(const LinearFitSample &o) const {
        return LinearFitSample{x - o.x, y - o.y, xx - o.xx, xy - o.xy};
    }

    T x;
    T y;
    T xx;
    T xy;
};
