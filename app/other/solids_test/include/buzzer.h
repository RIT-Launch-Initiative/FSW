#ifndef BUZZER_H
#define BUZZER_H

void buzzer_init();
void beep_full();

void test_start_beep();
void test_end_beep();

void set_ematch(int level);
void set_ldo(int level);

#endif // BUZZER_H