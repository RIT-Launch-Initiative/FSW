#ifndef FREAK_BUZZER_H
#define FREAK_BUZZER_H
enum BuzzCommand {
    Silent,
    LowBattery,
    DataLocked,

};
void buzzer_tell(enum BuzzCommand bc);
#endif