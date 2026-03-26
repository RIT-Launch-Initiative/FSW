#pragma once

#include <stdint.h>
#include <string.h>
#include <string>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

/**
 * Write timestamped log data to a file.
 * This is *not* for high speed telemetry data (for that, see Datalogger)
 * Instead, this is a human readable file detailing major events in the flight
 */
class CFlightLog {
  public:
    /**
     * Noop constructor - allows generating filename then initializing. MAKE SURE TO CALL A REAL CONSTRUCTOR BEFORE FURTHER USE
     */
    CFlightLog();
    /**
   * Open a flight log.
   * k_uptime_get() at time of opening is recorded as the first line of the flight log
   */
    explicit CFlightLog(const std::string& fname); /**
   * Open a flight log.
   * k_uptime_get() at time of opening is recorded as the first line of the flight log
   */
    explicit CFlightLog(const char* fname);
    /**
   * Open a flight log.
   * timestamp is user defined timestamp that marks the time of the flight log opening.
   */

    CFlightLog(const char* fname, int64_t timestamp);

    // Can't copy (would have two files with same name)
    CFlightLog(const CFlightLog&) = delete;
    CFlightLog(CFlightLog&&) = delete;

    /**
     * Destruct the log. 
     * Calls Close() to save file to disk
     */
    ~CFlightLog();

    /**
     * writes 'k_uptime_get(): msg' to the flight log
     * @param[in] msg zero terminated string message to write 
     */
    int Write(const char* msg);
    /**
     * writes 'timestamp: msg' to the flight log
     * @param timestamp  any user provided timestamp. It will be printed as a raw integer value
     * @param[in] msg zero terminated string message to write 
     */
    int Write(int64_t timestamp, const char* msg);

    /**
     * writes 'timestamp: msg' to the flight log
     * @param timestamp  any user provided timestamp. It will be printed as a raw integer value
     * @param[in] msg string message to write
     * @param str_len the length of the string to write
     */
    int Write(int64_t timestamp, const char* msg, size_t str_len);
    /**
     * Writes a message using the k_uptime_get (millis since boot) timestamp at time of calling
     * `k_uptime_get(): msg`
     * @param[in] msg string message to write
     * @param str_len the length of the string to write
     */
    int Write(const char* msg, size_t str_len);

    /**
     * Close the underlying file (called in destructor)
     * @return the error code from the file system
     */
    int Close();
    /**
     * Sync the underlying file to disk (like fflush)
     * The file will be synced when closed
     * @return the error code from the file system
     */
    int Sync();

  private:
    int writeTimestamp(int64_t timestamp);

    fs_file_t file;
};
