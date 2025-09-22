#pragma once
#include <cstdint>

enum class FiveVoltItem : uint8_t { Buzzer = 0, Servos = 1, Pump = 2, NumItems = 3 };
/*
5V Bus looks like this

                    +-- Servo Logic Shifter Enable---Servo Logic Shifters
VBAT---LDO Enable---+--       Buzzer Enable       ---Buzzer
                    +--       Pump Enable         ---Pump

The LDO draws a fair amount of current when running even when things downstream are off
This file allows controlling individual elements and automatically handling the LDO Enable pin 
*/

// enable a zone (if everything else is off, turn the rail on)
int rail_item_enable(FiveVoltItem item);

// disable a zone (if everything else is off, turn the rail off)
int rail_item_disable(FiveVoltItem item);

// change the state of a zone
int rail_item_set(FiveVoltItem item, bool state);
