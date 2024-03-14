# Requirements
- Nucleo-F446RE
- Launch Mikroe Click Shield
- W25Q128JV, connected to the shield's SPI with CS1

# Making a filesystem
First, declare the filesystem's parameters:

- Which filesystem is it - FAT or LittleFS?
- The storage device (in this case, flash device partition) to mount the
  filesystem to
- The mount point, which is the directory (string) identifying that filesystem.
  This allows the application to access different filesystems without each
  filesystem's individual API, just by changing the root directory. Up to
  `CONFIG_FILE_SYSTEM_MAX_TYPES=2` types of filesystem are allowed to be
  simultaneously used.
- The file system's numeric parameters (allocation sizes, cache sizes, etc.)
- Whether to automatically mount (prepare to use) the filesystem during boot-up.

A manually mounted filesystem needs a declaration in the application code. This
has two parts: the data, which is the configuration (cache sizes, etc) and the
mount point structure, which has a pointer to the data as one of its members. If
a filesystem is created in-tree under `fstab`, `FS_FSTAB_DECLARE_ENTRY` creates
both of these from the node's label. Otherwise, each filesystem provides
`DECLARE_CUSTOM_CONFIG` macros to create this structure, and then the mount
point contains a pointer to that structure.

# Shell
After all of the sample code finishes executing, the shell opens. The commands
are documented in the shell sample documentation.

# References:
- [LittleFS sample](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/fs/littlefs)
- [FS shell sample](https://docs.zephyrproject.org/latest/samples/subsys/shell/fs/README.html)
- [FSTAB binding](https://docs.zephyrproject.org/latest/build/dts/api/bindings/fs/zephyr,fstab,littlefs.html)
- [JEDEC NOR-SPI bindings](https://docs.zephyrproject.org/latest/build/dts/api/bindings/mtd/jedec%2Cspi-nor.html#std-dtcompatible-jedec-spi-nor)
- [Errno values](https://docs.zephyrproject.org/apidoc/latest/group__system__errno.html)
