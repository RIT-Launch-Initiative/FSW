#ifndef F_CORE_FLIGHT_FLIGHT_LOG_H
#define F_CORE_FLIGHT_FLIGHT_LOG_H
#include <stdint.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

/**
 * Write timestamped log data to a file.
 * Not for telemetry data (see Datalogger)
 * One off status updates
 */
class FlightLog {
  public:
    FlightLog(const char* fname, int64_t timestamp);
    explicit FlightLog(const char* fname);
    // Can't copy (would have two files with same name)
    FlightLog(const FlightLog&) = delete;
    FlightLog(FlightLog&&) = delete;

    /**
     * writes 'timestamp: msg' to the flight log
     * timestamp is any user provided timestamp. It will be printed as a raw integer value
     */
    int Write(int64_t timestamp, const char* msg);
    /**
     * Writes a message using the k_uptime_get (millis since boot) timestamp at time of calling
     */
    int Write(const char* msg);

  private:
    int writeTimestamp(int64_t timestamp);

    const char* filename;
    fs_file_t file;
};

#endif