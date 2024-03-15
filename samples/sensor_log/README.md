# Requirements
- Nucleo-F446RE
- Launch Mikroe Click Shield
- LIS3MDL, connected to the shield's I2C
- W25Q128JV, connected to the shield's SPI with CS1

# Current state
- Able to use filesystem to read a boot counter
- Figured out necessary kconfig and devicetree entries

# References:
- [LIS3MDL Kconfig options](https://docs.zephyrproject.org/latest/kconfig.html#!CONFIG_LIS3MDL)
- [JEDEC NOR-SPI bindings](https://docs.zephyrproject.org/latest/build/dts/api/bindings/mtd/jedec%2Cspi-nor.html#std-dtcompatible-jedec-spi-nor)
- [LittleFS sample](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/fs/littlefs)
- [FSTAB binding](https://docs.zephyrproject.org/latest/build/dts/api/bindings/fs/zephyr,fstab,littlefs.html)
- [Errno values](https://docs.zephyrproject.org/apidoc/latest/group__system__errno.html)
