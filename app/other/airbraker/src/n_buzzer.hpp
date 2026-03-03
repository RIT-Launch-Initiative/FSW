extern "C" int buzzer_init();
#include <cstdint>

namespace NBuzzer {
void MorseBlocking(uint32_t size, const char *str);

void SetBuzzer(bool buzz);

// Cancel NotFlying repeating forever when you acknowledge it over serial
void SilenceAlarm();

// repeatedly sound the not flying alarm until SilenceAlarm() is called
// after SilenceAlarm is 
void NotFlying();
void GoodToGoBlocking();

} // namespace NBuzzer