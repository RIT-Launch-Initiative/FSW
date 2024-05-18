#include <launch_core/conversions.h>

float l_altitude_conversion(float pressure_kpa, float temperature_c) {
    static const float R = 8.31447; // Universal gas constant in J/(mol·K)
    static const float g = 9.80665; // Standard acceleration due to gravity in m/s^2
    static const float M = 0.0289644; // Molar mass of Earth's air in kg/mol
    static const float L = 0.0065; // Temperature lapse rate in K/m
    static const float T0 = 288.15; // Standard temperature at sea level in K
    static const float P0 = 101325; // Standard pressure at sea level in Pa

    double temp = temperature_c + 273.15;
    double press = pressure_kpa * 1000; // Convert kPa to Pa
    return ((R * T0) / (g * M)) * log(P0 / press * pow((temp + L * 0.5) / T0, -g * M / (R * L)));
}
