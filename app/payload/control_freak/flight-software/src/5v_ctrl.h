#pragma once
#include <cstdint>

enum class FiveVoltItem : uint8_t { Buzzer = 0, Servos = 1, Pump = 2, NumItems = 3 };

// Prepares the GPIO
int five_volt_rail_init();

// enable a zone (if everything else is off, turn the rail on)
// NOTE: this does not turn on the buzzer/pump, just enables power to it
int rail_item_enable(FiveVoltItem item);

// disable a zone (if everything else is off, turn the rail off)
// NOTE: this does not turn off the buzzer/pump pin just handles the rail control
int rail_item_disable(FiveVoltItem item);

// disable a zone (if everything else is off, turn the rail off)
// NOTE: this does not turn on/off the buzzer/pump
int rail_item_set(FiveVoltItem item, bool state);
