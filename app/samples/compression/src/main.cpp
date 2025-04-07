#include <zephyr/kernel.h>
#include <n_autocoder_types.h>
#include <lz4.h>
#include <string>

static constexpr size_t MAX_BUFFER_SIZE = 256;

void printTestSet(std::string testSetName) {
    printk("----------------------");
    printk("%s", testSetName.c_str(), "");
    printk("----------------------");
}

void getCompressionStatistics(const std::string& type, void* data, int dataSize) {
    uint8_t compressedBuffer[MAX_BUFFER_SIZE] = {0};

    const int64_t startTime = k_uptime_get();
    const int compressedSize = LZ4_compress_default(static_cast<const char*>(data), reinterpret_cast<char*>(compressedBuffer), dataSize,MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;

    printk("\tType: %s - Compressed from %d to %d bytes in %ld milliseconds\n", type.c_str(), dataSize, compressedSize, elapsedTime);
}


int main() {
    NTypes::EnvironmentData envData = {0};
    NTypes::PowerData powerData = {0};
    NTypes::CoordinateData coordData = {0};

    printTestSet("Zeroed Data");
    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    printTestSet("Randomized data");
    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    printTestSet("Expected data");
    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    return 0;
}