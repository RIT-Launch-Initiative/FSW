#include <launch_core/conversions.h>

inline float l_alt_fr_press_temp(float pressure, float temperature) {
    // return ((pow((SEA_LEVEL_PRESSURE_BAR / pressure), PRESSURE_MAGIC_EXPONENT) - 1.0f) * (temperature + CELSIUS_TO_KELVIN)) / HYPSOMETRIC_FORMULA_DENOMINATOR;
    return 0.0f;
}