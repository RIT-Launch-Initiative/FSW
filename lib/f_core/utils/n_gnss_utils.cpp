#include "f_core/utils/n_gnss_utils.h"

void NGnssUtils::PopulateGnssNavigationData(const navigation_data* data, GnssCoordinates* coordinates) {
    // Convert nanodegrees to degrees
    coordinates->latitude = data->latitude / 1000000000.0f;
    coordinates->longitude = data->longitude / 1000000000.0f;

    // Convert millimeters to meters
    coordinates->altitude = data->altitude / 1000.0f; ;
}

// TODO: Add more implementations for each grouping

void NGnssUtils::PopulateGnssStruct(const gnss_data* data, GnssData* gnssData) {
    PopulateGnssNavigationData(&data->nav_data, &gnssData->coordinates);
}