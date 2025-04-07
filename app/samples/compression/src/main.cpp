#include <zephyr/kernel.h>
#include <n_autocoder_types.h>
#include <lz4.h>
#include <string.h>
#include <string>
#include <random>

static constexpr size_t MAX_BUFFER_SIZE = 256;
static constexpr size_t pretendFlightFileSize = sizeof(NTypes::EnvironmentData) * 100 * 60 * 3;
static uint8_t fileBuffer[pretendFlightFileSize];
static uint8_t compressedFileBuffer[pretendFlightFileSize];

static constexpr void randomizeFileBuffer() {
    // Gives less 0s if we initialize the buffer with some data
    NTypes::EnvironmentData realisticEnvData {
        .PrimaryBarometer = {
            .Pressure = 1013.59,
            .Temperature = -19.0,
        },
        .SecondaryBarometer = {
            .Pressure = 998.45,
            .Temperature = -22.0,
        },
        .Acceleration = {
            .X = 6.13,
            .Y = 9.37,
            .Z = 50.0,
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
        .Magnetometer = {
            .X = 0.37,
            .Y = 0.04,
            .Z = 1.2,
        },
        .Temperature = -20.0,
    };

    for (size_t i = 0; i < pretendFlightFileSize / sizeof(NTypes::EnvironmentData); i++) {
        memcpy(&fileBuffer[i * sizeof(NTypes::EnvironmentData)], &realisticEnvData, sizeof(NTypes::EnvironmentData));
        // Change up some numbers
        reinterpret_cast<NTypes::EnvironmentData*>(&fileBuffer[i * sizeof(NTypes::EnvironmentData)])->PrimaryBarometer.Pressure += static_cast<float>(rand() % 100) / 100.0f;
    }
}

void printTestSet(std::string testSetName) {
    printk("----------------------\n");
    printk("%s\n", testSetName.c_str());
    printk("----------------------\n");
}

void getDecompressionSafeStatistics(const char* type, void* compressedData, void *uncompressedData, const size_t dataSize) {
    uint8_t decompressedBuffer[MAX_BUFFER_SIZE] = {0};
    const int64_t startTime = k_uptime_get();
    const int decompressedSize = LZ4_decompress_safe(reinterpret_cast<const char*>(compressedData),
                                                     reinterpret_cast<char*>(decompressedBuffer), dataSize,
                                                     MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;
    int errorCount = 0;
    for (int i = 0; i < decompressedSize; ++i) {
        if (decompressedBuffer[i] != static_cast<uint8_t*>(uncompressedData)[i]) {
            errorCount++;
        }
    }

    printk("\t\tDecompressed from %zu to %zu bytes in %ld milliseconds with %d errors\n", dataSize, decompressedSize, elapsedTime, errorCount);
}

void getCompressionSafeStatistics(const char* type, void* data, const size_t dataSize) {
    uint8_t compressedBuffer[MAX_BUFFER_SIZE] = {0};
    const int64_t startTime = k_uptime_get();
    const int compressedSize = LZ4_compress_default(static_cast<const char*>(data),
                                                    reinterpret_cast<char*>(compressedBuffer), dataSize,
                                                    MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;

    printk("\t%s:\n", type);
    printk("\t\tCompressed from %d to %d bytes in %ld milliseconds\n", dataSize, compressedSize, elapsedTime);
    getDecompressionSafeStatistics(type, compressedBuffer, data, compressedSize);
}

void getDecompressionUnsafeStatistics(const char* type, void* compressedData, void *uncompressedData, const size_t dataSize, const size_t uncompressedSize) {
    uint8_t decompressedBuffer[MAX_BUFFER_SIZE] = {0};
    const int64_t startTime = k_uptime_get();
    const int decompressedSize = LZ4_decompress_safe_partial(reinterpret_cast<const char*>(compressedData),
                                                              reinterpret_cast<char*>(decompressedBuffer), dataSize,
                                                              10, MAX_BUFFER_SIZE);
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;
    int errorCount = 0;
    for (int i = 0; i < decompressedSize; ++i) {
        if (decompressedBuffer[i] != static_cast<uint8_t*>(uncompressedData)[i]) {
            errorCount++;
        }
    }

    printk("\t\tDecompressed from %zu to %zu bytes in %ld milliseconds with %d errors\n", dataSize, decompressedSize, elapsedTime, errorCount);
}

void getCompressionUnsafeStatistics(const char* type, void* data, const size_t dataSize) {
    uint8_t compressedBuffer[MAX_BUFFER_SIZE] = {0};
    const int64_t startTime = k_uptime_get();
    const int64_t endTime = k_uptime_get();
    const int64_t elapsedTime = endTime - startTime;

    const int decompressedSize = LZ4_compress(static_cast<const char*>(data), reinterpret_cast<char*>(compressedBuffer), dataSize);
    printk("\t%s:\n", type);
    printk("\t\tCompressed from %zu to %zu bytes in %ld milliseconds\n", dataSize, dataSize, elapsedTime);
    getDecompressionUnsafeStatistics(type, data, data, dataSize, decompressedSize);
}


int main() {
    NTypes::EnvironmentData envData;
    NTypes::PowerData powerData;
    NTypes::CoordinateData coordData;

    randomizeFileBuffer();
    for (int i =0 ; i < pretendFlightFileSize; ++i) {
        printk("%d ", fileBuffer[i]);
    }

    printTestSet("Zeroed Data");
    memset(&envData, 0, sizeof(NTypes::EnvironmentData));
    memset(&powerData, 0, sizeof(NTypes::PowerData));
    memset(&coordData, 0, sizeof(NTypes::CoordinateData));

    getCompressionSafeStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionSafeStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionSafeStatistics("CoordinateData", &coordData, sizeof(coordData));

    printTestSet("Unsafe Zeroed Data");
    getCompressionUnsafeStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionUnsafeStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionUnsafeStatistics("CoordinateData", &coordData, sizeof(coordData));

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

    getCompressionSafeStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionSafeStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionSafeStatistics("CoordinateData", &coordData, sizeof(coordData));

    printTestSet("Unsafe Random Data");
    getCompressionUnsafeStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionUnsafeStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionUnsafeStatistics("CoordinateData", &coordData, sizeof(coordData));


    printTestSet("Expected data");
    NTypes::EnvironmentData realisticEnvData {
        .PrimaryBarometer = {
            .Pressure = 1013.59,
            .Temperature = -19.0,
        },
        .SecondaryBarometer = {
            .Pressure = 998.45,
            .Temperature = -22.0,
        },
        .Acceleration = {
            .X = 6.13,
            .Y = 9.37,
            .Z = 50.0,
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
        .Magnetometer = {
            .X = 0.37,
            .Y = 0.04,
            .Z = 1.2,
        },
        .Temperature = -20.0,
    };
    NTypes::PowerData realisticPowerData{
        // I actually don't know current and voltage without reading actual sensors
        .RailBattery = {
            .Voltage = 12.0,
            .Current = 13.37,
            .Power = 6.0,
        },
        .Rail3v3 = {
            .Voltage = 3.3,
            .Current = 0.5,
            .Power = 1.0,
        },
        .Rail5v0 = {
            .Voltage = 5.0,
            .Current = 1,
            .Power = 2.5,
        },
    };
    NTypes::CoordinateData realisticCoordData = {
        .Latitude = 43.08,
        .Longitude = -77.67,
        .Altitude = 1000,
    };

    getCompressionSafeStatistics("EnvironmentData", &realisticEnvData, sizeof(realisticEnvData));
    getCompressionSafeStatistics("PowerData", &realisticPowerData, sizeof(realisticPowerData));
    getCompressionSafeStatistics("CoordinateData", &realisticCoordData, sizeof(realisticCoordData));

    printTestSet("Unsafe Expected Data");
    getCompressionUnsafeStatistics("EnvironmentData", &envData, sizeof(envData));
    getCompressionUnsafeStatistics("PowerData", &powerData, sizeof(powerData));
    getCompressionUnsafeStatistics("CoordinateData", &coordData, sizeof(coordData));


    printTestSet("Pretend file");
    getCompressionSafeStatistics("File", &fileBuffer, sizeof(fileBuffer));
    getCompressionUnsafeStatistics("File", &fileBuffer, sizeof(fileBuffer));

    return 0;
}