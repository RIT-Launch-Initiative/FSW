#ifndef FREAK_BUZZER_H
#define FREAK_BUZZER_H
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
#endif