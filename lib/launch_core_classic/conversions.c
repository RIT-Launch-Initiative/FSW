#include <launch_core_classic/conversions.h>

double l_altitude_conversion(double pressure_kpa, double temperature_c) {
    static const double R = 8.31447; // Universal gas constant in J/(mol·K)
    static const double g = 9.80665; // Standard acceleration due to gravity in m/s^2
    static const double M = 0.0289644; // Molar mass of Earth's air in kg/mol
    static const double L = 0.0065; // Temperature lapse rate in K/m
    static const double T0 = 288.15; // Standard temperature at sea level in K
    static const double P0 = 101325; // Standard pressure at sea level in Pa

    double temp = temperature_c + 273.15;
    double press = pressure_kpa * 1000; // Convert kPa to Pa
    return ((R * T0) / (g * M)) * log(P0 / press * pow((temp + L * 0.5) / T0, -g * M / (R * L)));
}
