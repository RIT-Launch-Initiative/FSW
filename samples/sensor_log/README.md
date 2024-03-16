# Requirements
- Nucleo-F446RE
- Launch Mikroe Click Shield
- LIS3MDL, connected to the shield's I2C
- W25Q128JV, connected to the shield's SPI with CS1

# Demonstrates
- Using a sensor logger in circular mode 
- Using a threshold to start logging in once-mode

# Known issues
- Can't pre-allocate the file to check whether there is enough space, and
  growing a file appears to be quadratic. `fs_truncate` is very slow (5m36s to
  fill up the chip), so we need to figure out whether that will translate to
  taking a very long time to grow to that point.
