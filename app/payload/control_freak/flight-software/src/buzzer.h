#pragma once
void buzzer_entry_point(void *, void *, void *);

enum BuzzCommand {
    Silent,
    AllGood,
    SensorTroubles,
    BatteryWarning,
    BatteryBad,
    DataLocked,

};
void buzzer_tell(enum BuzzCommand bc);
