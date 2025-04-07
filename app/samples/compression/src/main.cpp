#include <zephyr/kernel.h>
#include <n_autocoder_types.h>
#include <lz4.h>
#include <string.h>
#include <string>
#include <random>

static constexpr size_t MAX_BUFFER_SIZE = 256;

void printTestSet(std::string testSetName) {
    printk("----------------------\n");
    printk("%s\n", testSetName.c_str());
    printk("----------------------\n");
}

void getDecompressionStatistics(const std::string& type, void* data, const int dataSize) {
    uint8_t compressedBuffer[MAX_BUFFER_SIZE] = {0};

    const int64_t startTime = k_uptime_get();
    const int decompressedSize = LZ4_decompress_safe(reinterpret_cast<const char*>(data), reinterpret_cast<char*>(compressedBuffer), dataSize, MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;

    printk("\t\tDecompressed from %zu to %zu bytes in %ld milliseconds\n", type.c_str(), dataSize, decompressedSize, elapsedTime);
}

void getCompressionStatistics(const std::string& type, void* data, const int dataSize) {
    uint8_t compressedBuffer[MAX_BUFFER_SIZE] = {0};

    const int64_t startTime = k_uptime_get();
    const int compressedSize = LZ4_compress_default(static_cast<const char*>(data), reinterpret_cast<char*>(compressedBuffer), dataSize,MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;

    printk("\t%s:\n", type.c_str());
    printk("\t\tCompressed from %zu to %zu bytes in %ld milliseconds\n", type.c_str(), dataSize, compressedSize, elapsedTime);
    getDecompressionStatistics(type, compressedBuffer, compressedSize);
}


int main() {
    NTypes::EnvironmentData envData;
    NTypes::PowerData powerData;
    NTypes::CoordinateData coordData;


    printTestSet("Zeroed Data");
    memset(&envData, 0, sizeof(NTypes::EnvironmentData));
    memset(&powerData, 0, sizeof(NTypes::PowerData));
    memset(&coordData, 0, sizeof(NTypes::CoordinateData));
    printk("sizeof EnvironmentData: %zu\n", sizeof(NTypes::EnvironmentData));

    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    printTestSet("Randomized data");
    srand(0);
    for (int i = 0; i < sizeof(NTypes::EnvironmentData); ++i) {
        reinterpret_cast<uint8_t*>(&envData)[i] = static_cast<uint8_t>(rand() % 256);
    }

    for (int i = 0; i < sizeof(NTypes::PowerData); ++i) {
        reinterpret_cast<uint8_t*>(&powerData)[i] = static_cast<uint8_t>(rand() % 256);
    }

    for (int i = 0; i < sizeof(NTypes::CoordinateData); ++i) {
        reinterpret_cast<uint8_t*>(&coordData)[i] = static_cast<uint8_t>(rand() % 256);
    }

    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    printTestSet("Expected data");
    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    return 0;
}