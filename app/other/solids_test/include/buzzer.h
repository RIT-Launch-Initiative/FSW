#ifndef BUZZER_H
#define BUZZER_H

int buzzer_init(void);
void beep_full();

void test_start_beep();
void test_end_beep();

void set_ematch(int level);
void set_ldo(int level);

#endif // BUZZER_H