#pragma once
#include <cstdint>
#include <zephyr/kernel.h>

class CycleCounter {
  public:
    CycleCounter(int64_t &toAdd) : toAdd(toAdd) { start_cyc = k_cycle_get_64(); }
    ~CycleCounter() { toAdd += k_cycle_get_64() - start_cyc; }

    CycleCounter(CycleCounter &&) = delete;
    CycleCounter(const CycleCounter &) = delete;
    CycleCounter &operator=(CycleCounter &&) = delete;
    CycleCounter &operator=(const CycleCounter &) = delete;

  private:
    int64_t &toAdd;
    int64_t start_cyc;
};
