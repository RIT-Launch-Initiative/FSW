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

void getDecompressionStatistics(const char* type, void* data, const size_t dataSize) {
    uint8_t compressedBuffer[MAX_BUFFER_SIZE] = {0};
    const int64_t startTime = k_uptime_get();
    const int decompressedSize = LZ4_decompress_safe(reinterpret_cast<const char*>(data),
                                                     reinterpret_cast<char*>(compressedBuffer), dataSize,
                                                     MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;

    printk("\t\tDecompressed from %zu to %zu bytes in %ld milliseconds\n", dataSize, decompressedSize, elapsedTime);
}

void getCompressionStatistics(const char* type, void* data, const size_t dataSize) {
    uint8_t compressedBuffer[MAX_BUFFER_SIZE] = {0};
    const int64_t startTime = k_uptime_get();
    const int compressedSize = LZ4_compress_default(static_cast<const char*>(data),
                                                    reinterpret_cast<char*>(compressedBuffer), dataSize,
                                                    MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;

    printk("\t%s:\n", type);
    printk("\t\tCompressed from %d to %d bytes in %ld milliseconds\n", dataSize, compressedSize, elapsedTime);
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
    for (uint32_t i = 0; i < sizeof(NTypes::EnvironmentData); ++i) {
        reinterpret_cast<uint8_t*>(&envData)[i] = static_cast<uint8_t>(rand() % 256);
    }

    for (uint32_t i = 0; i < sizeof(NTypes::PowerData); ++i) {
        reinterpret_cast<uint8_t*>(&powerData)[i] = static_cast<uint8_t>(rand() % 256);
    }

    for (uint32_t i = 0; i < sizeof(NTypes::CoordinateData); ++i) {
        reinterpret_cast<uint8_t*>(&coordData)[i] = static_cast<uint8_t>(rand() % 256);
    }

    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    printTestSet("Expected data");
    NTypes::EnvironmentData realisticEnvData {
        .Acceleration = {
            .X = 6.13,
            .Y = 9.37,
            .Z = 50.0,
        },
        .Magnetometer = {
            .X = 0.37,
            .Y = 0.04,
            .Z = 1.2,
        },
        .PrimaryBarometer = {
            .Pressure = 1013.59,
            .Temperature = -19.0,
        },
        .SecondaryBarometer = {
            .Pressure = 998.45,
            .Temperature = -22.0,
        },
        .ImuAcceleration = {
            .X = 8.13,
            .Y = 12.37,
            .Z = 47.24,
        },
        .ImuGyroscope = {
            .X = -50.0,
            .Y = 50.0,
            .Z = 15.0,
        },
        .Temperature = -20.0,
    };
    NTypes::PowerData realisticPowerData{
        // I actually don't know current and voltage without reading actual sensors
        .Rail3v3 = {
            .Current = 0.5,
            .Voltage = 3.3,
            .Power = 1.0,
        },
        .Rail5v0 = {
            .Current = 1,
            .Voltage = 5.0,
            .Power = 2.5,
        }
        .RailBattery = {
            .Current = 13.37,
            .Voltage = 12.0,
            .Power = 6.0,
        },
    };
    NTypes::CoordinateData realisticCoordData = {
        .Latitude = 43.08,
        .Longitude = -77.67,
        .Altitude = 1000,
    };

    getCompressionStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionStatistics("CoordinateData", &coordData, sizeof(coordData));

    return 0;
}