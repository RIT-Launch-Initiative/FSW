#ifndef C_DATALOGGER_H
#define C_DATALOGGER_H

#ifndef CONFIG_F_CORE_OS
#error "In order to use these APIs, set CONFIG_F_CORE_OS=y"
#endif

#include <cstdint>
#include <type_traits>
#include <zephyr/fs/fs.h>

enum class LogMode { Growing, Circular, FixedSize };

/// @brief Internal, type-unsafe datalogger. Don't use this directly. Use CDataLogger instead
namespace detail {
class datalogger {
  public:
    datalogger(const char *filename, LogMode mode, std::size_t num_packets);
    int write(const void *data, std::size_t size);
    int close();

    const char *filename;
    fs_file_t file;
    LogMode mode;
    std::size_t num_packets;
};
} // namespace detail

template <typename T> class CDataLogger {
  public:
    using PacketType = T;
    static_assert(std::is_trivial<PacketType>::value,
                  "You probably don't want to serialize non-trivial types this way"
                  "If the packet is non trivial, it has special rules about how to create it and destroy it."
                  "When reading the saved data, we no longer have that information so wouldn't be able to reconstruct "
                  "the Packet correctly");
    static_assert(std::is_standard_layout<PacketType>::value,
                  "Non standard layout types will not serialize correctly"
                  "Non standard layout types can have vtable pointers or other such nonsense in their layout."
                  "You should use a simple struct for your packet instead");

  public:
    CDataLogger(const char *filename) : internal(filename, LogMode::Growing, 0) {}
    CDataLogger(const char *filename, LogMode mode, std::size_t num_packets) : internal(filename, mode, num_packets) {}
    int write(const PacketType &packet) {
        return internal.write(reinterpret_cast<const void *>(&packet), sizeof(PacketType));
    }
    /**
     * Close the file and flush to disk.
     * Make sure to do this or some of your data may not be sent to the disk before power is cut/the chip is turned off
     * @retval 0 on success;
     * @retval -ENOTSUP when not implemented by underlying file system driver;
     * @retval <0 a negative errno code on error.
     */
    int close() { return internal.close(); }

  private:
    detail::datalogger internal;
};

#endif