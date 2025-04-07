#include <zephyr/kernel.h>
#include <n_autocoder_types.h>
#include <lz4.h>
#include <string>


void printTestSet(std::string testSetName) {
    printk("----------------------");
    printk("%s", testSetName.c_str(), "");
    printk("----------------------");
}

void getCompressionStatistics(std::string type, void *data, size_t dataSize) {


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