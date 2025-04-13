#ifndef N_GNSS_UTILS_H
#define N_GNSS_UTILS_H

#include <zephyr/drivers/gnss.h>



namespace NGnssUtils {
     struct __attribute__((packed)) GnssCoordinates {
        double latitude;
        double longitude;
        float altitude;
    };

    struct __attribute__((packed)) GnssTime {
        uint32_t time;
        uint32_t date;
    };

    struct __attribute__((packed)) GnssInfo {
        uint8_t satelliteId;
        uint8_t elevation;
        uint16_t azimuth;
        uint8_t snr;
    };

    struct __attribute__((packed)) GnssData {
        GnssCoordinates coordinates;
        GnssTime time;
        GnssInfo info;
    };

    void PopulateGnssNavigationData(const navigation_data* data, GnssCoordinates* coordinates);

    void PopulateGnssInfoData(const gnss_info* data, GnssInfo* satellites);

    void PopulateGnssStruct(const gnss_data* data, GnssData* gnssData);
    
};



#endif // N_GNSS_UTILS_H
