extern "C" int buzzer_init();
#include <cstdint>

namespace NBuzzer {
/**
 * Speak in morse code 
 * Symbols: .- / (dot, dash, between letter, between word)
 */
void MorseBlocking(uint32_t size, const char *str);

/**
 * Cancel NotFlying alarm causing any code running that to return asap 
 */
void SilenceAlarm();

/**
 * repeatedly sound the not flying alarm until SilenceAlarm() is called
 * after SilenceAlarm is called, this will return so make sure you know what happens after it returns
 */
void NotFlying();

/**
 * Play happy sound saying the board is on
 */
void GoodToGoBlocking();

} // namespace NBuzzer