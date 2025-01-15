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
    /**
   * Open a flight log.
   * k_uptime_get() at time of opening is recorded as the first line of the flight log
   */
    explicit FlightLog(const char* fname);
    /**
   * Open a flight log.
   * timestamp is user defined timestamp that marks the time of the flight log opening.
   */

    FlightLog(const char* fname, int64_t timestamp);
    // Can't copy (would have two files with same name)
    FlightLog(const FlightLog&) = delete;
    FlightLog(FlightLog&&) = delete;

    ~FlightLog();

    /**
     * writes 'k_uptime_get(): msg' to the flight log
     * @param timestamp  any user provided timestamp. It will be printed as a raw integer value
     * @param msg zero terminated string message to write 
     */
    int Write(const char* msg);
    /**
     * writes 'timestamp: msg' to the flight log
     * @param timestamp  any user provided timestamp. It will be printed as a raw integer value
     * @param msg zero terminated string message to write 
     */
    int Write(int64_t timestamp, const char* msg);

    /**
     * writes 'timestamp: msg' to the flight log
     * @param timestamp  any user provided timestamp. It will be printed as a raw integer value
     * @param msg string message to write
     * @param str_len the length of the string to write
     */
    int Write(int64_t timestamp, const char* msg, size_t str_len);
    /**
     * Writes a message using the k_uptime_get (millis since boot) timestamp at time of calling
     * `k_uptime_get(): msg`
     * @param msg string message to write
     * @param str_len the length of the string to write
     */
    int Write(const char* msg, size_t str_len);

    int Close();
    int Sync();

  private:
    int writeTimestamp(int64_t timestamp);

    fs_file_t file;
};

#endif