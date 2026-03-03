extern "C" int buzzer_init();

namespace NBuzzer {
void MorseBlocking(uint32_t size, const char *str);

void SetBuzzer(bool buzz);

void NogoBlocking();

} // namespace NBuzzer