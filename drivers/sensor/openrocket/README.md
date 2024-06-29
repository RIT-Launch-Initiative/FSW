# Openrocket Simulation Sensors

Currently Supported Sensors:
- None

Planned Supported Sensors:
- 6DOF IMU
- Altimeter (Pressure, Temperature)
- GNSS device
    - Simulated Lat, Long
    - Altitude from sim -> GPS altitude above launch start location


# General Configuration
The file to use is specified by the Kconfig symbol 

## Data File
`CONFIG_OPENROCKET_DATA_FILE`. At build time, CMake runs a script to convert this CSV into a C file that can be compiled into the final binary. 

## Launch Delay
`CONFIG_OPENROCKET_LAUNCH_DELAY`. How long to wait until starting the openrocket time. Until this point, the sensors will read the T=0 value plus any noise configured on a sensor by sensor basis. 

# Sensor Configuration 

## IMU Configuration 
### Device Tree Configuration

#### vertical-axis
Type: Enum
`vertical-axis` controls which accelerometer axis is the upwards (direction the rocket goes) axis.
Possible values:
- POS_X
- POS_Y
- POS_Z 
- NEG_X
- NEG_Y
- NEG_Z 

#### accel-noise
`accel-noise`

#### vertical-axis-bias

#### is-broken
Type: Boolean

Sometimes, your sensors break. If this option is selected, the device will return false on a call to `device_is_ready` and will not execute any `sensor_channel_fetch` requests. 

#### sampling-period-us
Type: Number

What frequency does the sensor update at. The simulated sensor will only update at this rate no matter how the underlying data changes beneath it or how often the use calls sensor_channel. A value of 0 (the default) signifies the the value will interpolated to read as fast as `sensor_channel_fetch` is called. 

## Altimeter Configuration 

## GNSS Configuration 