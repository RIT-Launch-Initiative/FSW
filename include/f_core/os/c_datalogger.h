#include <cstdint>
#include <type_traits>
#include <zephyr/fs/fs.h>

enum class LogMode { Growing, Circular, FixedSize };

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
    /// If the packet is non trivial, it has special rules about how to create it and destroy it.
    /// When reading the saved data, we no longer have that information so wouldn't be able to reconstruct the Packet correctly
    static_assert(std::is_trivial<PacketType>::value,
                  "You probably don't want to serialize non-trivial types this way");
    /// Non standard layout types can have vtable pointers or other such nonsense in their layout.
    /// You should use a simple struct for your packet rather than serializing a complicated class
    static_assert(std::is_standard_layout<PacketType>::value, "Non standard layout types will not serialize correctly");

  public:
    CDataLogger(const char *filename) : internal(filename, LogMode::Growing, 0) {}
    CDataLogger(const char *filename, LogMode mode, std::size_t num_packets) : internal(filename, mode, num_packets) {}
    int write(const PacketType &packet) {
        return internal.write(reinterpret_cast<const void *>(&packet), sizeof(PacketType));
    }
    int close() { return internal.close(); }

  private:
    detail::datalogger internal;
};