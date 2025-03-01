#include <f_core/utils/circular_buffer.hpp>

#include <array>
#include <cstddef>

template <typename T, std::size_t len>
class CRollingSum {
  public:
    static_assert(len > 0, "What is the sum of 0 elements? You probably don't want this (also it will break)");

    using value_type = T;

    constexpr CRollingSum(T start) : buf(start) { Fill(start); }

    constexpr std::size_t Size() const { return size; }

    constexpr void Fill(const value_type &start) {
        buf.Fill(start);
        total = buf.OldestSample();
        for (std::size_t i = 1; i < buf.Size(); i++) {
            total = total + buf[i];
        }
    }

    constexpr void Feed(const value_type &new_value) {
        value_type oldest = buf.OldestSample();
        total = total - oldest;
        total = total + new_value;
        buf.AddSample(new_value);
    }
    constexpr value_type Sum() const { return total; }

  private:
    static constexpr std::size_t size = len;

    value_type total;
    CCircularBuffer<value_type, size> buf;
};

template <typename T, std::size_t len>
class CMovingAverage {
  public:
    using value_type = T;
    static constexpr std::size_t length = len;

    constexpr CMovingAverage(value_type start) : rolling_sum(start) {}

    constexpr void Feed(value_type value) { rolling_sum.Feed(value); }
    constexpr value_type Avg() { return rolling_sum.Sum() / (value_type) len; }
    constexpr void Fill(const T &start) { rolling_sum.Fill(start); }

  private:
    CRollingSum<value_type, len> rolling_sum;
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

template <typename T, std::size_t window_size>
bool find_slope(const CRollingSum<LinearFitSample<T>, window_size> &data, T &slope) {
    std::size_t N = data.Size();
    LinearFitSample<T> E = data.Sum();
    T denom = (N * E.xx - E.x * E.x);

    if (denom == 0) {
        // Would have divided by zero
        return false;
    }
    slope = (N * E.xy - E.x * E.y) / denom;
    return true;
}
