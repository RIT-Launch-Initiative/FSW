#ifndef C_DATALOGGER_H
#define C_DATALOGGER_H

#include <cstdint>
#include <type_traits>
#include <zephyr/fs/fs.h>

enum class LogMode { Growing, Circular, FixedSize };

/// @brief Internal, type-unsafe datalogger. Don't use this directly. Use CFsDataLogger instead
namespace detail {
class datalogger {
  public:
    datalogger(const char *filename, LogMode mode, std::size_t num_packets);
    int write(const void *data, std::size_t size);
    int close();
    int sync();

    const char *filename;
    fs_file_t file;
    LogMode mode;
    std::size_t num_packets;
};
} // namespace detail

/**
 * @brief A type-safe class that supports writing fixed-sized packets to the filesystem
 * The Datalogger can be configured to use different modes:
 * - Growing - the file will grow ever larger as you write more packets (assuming space is still available on the device) 
 * - Circular - the file will hold only a certain number of packets. Old data will be overwritten with new data
 * - FixedSize - the file will hold only a certain number of packets. Old data will be retained if you try to write more packets than it can fit
 * This class is implemented as a type safe wrapper to detail::datalogger.
 */
template <typename T>
class CFsDataLogger {
  public:
    using PacketType = T;
    static_assert(std::is_trivially_copyable<PacketType>::value,
                  "Only trivially copyable types can be known to serialize and deserialize correctly generically. A "
                  "simple struct is trivially copyable - you should probably be using one of those. For more "
                  "information, see https://en.cppreference.com/w/cpp/types/is_trivially_copyable");

  public:
    /**
     * Construct a Datalogger for the specified filename
     * The logger will use the "Growing" mode and will expand as you write more data until your filesystem runs out of space.
     * @param filename the name of the file to write to
     */
    CFsDataLogger(const char *filename) : internal(filename, LogMode::Growing, 0) {}
    /**
     * Construct a Datalogger for the specified filename, grow mode, and size
     * @param filename the name of the file to write to
     * @param mode the logging mode to use
     * @param the number of packets to log (only used if mode is Circular or FixedSize)
     */
    CFsDataLogger(const char *filename, LogMode mode, std::size_t num_packets) : internal(filename, mode, num_packets) {}

   /**
     * Write a packet to the file
     * @param packet the data to write to the file
     */
    int Write(const PacketType &packet) {
        return internal.write(reinterpret_cast<const void *>(&packet), sizeof(PacketType));
    }

    /**
     * Close the file and flush to disk.
     * Make sure to do this or some of your data may not be sent to the disk before power is cut/the chip is turned off
     * @retval 0 on success;
     * @retval -ENOTSUP when not implemented by underlying file system driver;
     * @retval <0 a negative errno code on error.
     */
    int Close() { return internal.close(); }

    /**
     * Sync the file to flash/disk.
     *
     * @return <0 a negative errno code on error.
     */
    int Sync() { return internal.sync(); }

  private:
    detail::datalogger internal;
};

#endif
