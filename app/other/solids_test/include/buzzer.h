#ifndef BUZZER_H
#define BUZZER_H

/**
 * @brief Configure buzzer and ldo gpio pins
 * @return 0 if successful
 */
int buzzer_init();

/**
 * @brief Start or stop buzzer
 * @param which Value to set buzzer
 */
void set_buzz(int which);

/**
 * @brief Set ematch gpio pin
 * @param level Value to assign to pin
 */
void set_ematch(int level);

/**
 * @brief Set ldo gpio pin
 * @param level Value to assign to pin
 */
void set_ldo(int level);

/**
 * @brief Beep loudly in 1 second intervals for 10 seconds to indicate that flash is full (max tests reached)
 */
void beep_full();

/**
 * @brief Beep loudly 3 times to indicate test start
 */
void test_start_beep();

/**
 * @brief Beep loudly 2 times to indicate test end
 */
void test_end_beep();

/**
 * @brief Beep forever
 */
void continuous_beep();

#endif // BUZZER_H