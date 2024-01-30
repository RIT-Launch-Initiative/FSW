# Requirements
- Nucleo-F446RE
- Launch Mikroe Click Shield
- W25Q128JV, connected to the shield's SPI with CS1

# Usage
All commands (cd, pwd, etc.) need to have `fs` in front of them. By default,
there is a LittleFS at `/lfs`. The shell sample (see references) documents the
commands.

# Remarks
- The `fstab` node defines a LittleFS auto-mounted at `/lfs`, if this is not
  present, you will need to mount a filesystem with `fs mount littlefs
  <mount_point>`, where `<mount_point>` is something like `/lfs`, `/storage`,
  etc.
- The file system shell mounts new filesystems to the node `storage_partition`.
  This is not configurable at time of writing. The Nucleo's device tree
  currently defines a `storage_partition` on the STM32's flash, which we cannot
  mount a filesystem to, so either
    - Stick to filesystems that are auto-mounted or mounted before `main` returns.
    - Put `/delete-node/ &storage_partition` after `/ {}` to remove the STM32
      partition and create a `fixed_partition` on the external flash named
      `storage_partition`.
  

# References:
- [FS shell sample](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/shell/fs)
