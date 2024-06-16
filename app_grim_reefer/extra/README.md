# User Guide

## Configuration
Here are some configurations you might want to change during testing. Unless otherwise specified they are located in `app_grim_reefer/src/config.h`.
### Control
#### `EASY_BOOST_DETECT`
Flight Config: `off`
If this is defined, boost detect will look for 5G of accelaration. Depending on your choice of boost detection mode, this wlil either boost detect immediatly (magnitude mode) or detect if you orient the board as if it was in flight. 
#### `SHORT_FLIGHT`
Flight Config: `off`
If this is defined, use shorter timers for the flight length, and camera shutoff time. If not, use the official flight time and camera shutoff time
#### `BUZZER_USE_LED`
Flight Config: `off`

If this is defined, the buzzer o

#### `IMU_BOOST_DETECTION_MODE_AXIS`
If this is defined, use the axis specified by `IMU_UP_AXIS` for boost detection. If undefined use magnitude. 

#### `IMU_UP_AXIS`
Which axis to use if using single axis boost detection. One of `accel_x`, `accel_y`, or `accel_z`. 

### Flight Parameters

#### `FLIGHT_LENGTH`
Flight Config: `K_SECONDS(400)`

> [!WARNING] Make sure you change the correct `TOTAL_FLIGHT_TIME`
> One instance of the `TOTAL_FLIGHT_TIME` is for the short, debugging flight and one is for the full flight.
#### `CAMERA_EXTRA_TIME`
Flight Config: `K_MINUTES(12)`

> [!WARNING] Make sure you change the correct `CAMERA_EXTRA_TIME`
> One instance of the `CAMERA_EXTRA_TIME` is for the short, debugging flight and one is for the full flight.

#### `ACCEL_VAL_THRESHOLD`
Flight Config `(float)(5 * 9.81)` - 5G
What acceleration value must we exceed in order to detect boost. 

#### `ACCEL_BUFFER_SIZE`
The number of samples to detect boost with. See the [[#Boost Detect Buffer Size Test]] for more information about how to use this. 

#### Debug Build

Will print helpful information about the system to the UART console. 
> [!WARNING] Caution
> This can modify how fast things run since the system cant always meet its targets, not recommended for the [[#I2C lockup test]] or [[#Boost Detect Buffer Size Test]].



# Testing

## I2C lockup test
1. [[#Software Deploy|Deploy the code]]
2. let the board run
3. power cycle
4. let the board run
5. repeat until comfortable
6. If you hear [[#Beepcode Missing Sensors]] consistently, raise the value of `FAST_DATA_DELAY`. If you change this, make sure you run [[#Boost Detect Buffer Size Test]]
 
## Axis IMU Detection Test
Configuration: 
- `EASY_BOOST_DETECT` is defined
- `IMU_BOOST_DETECTION_MODE_AXIS` is defined
Procedure
1. place the grim in the payload in flight mounting position
2. place the payload on its side
3. [[#Software Deploy|Deploy software]] and let it run
4. wait 10 seconds to make sure detection is not erroneously triggered by the altitude. (if this happens, call 781-812-8902 something very bad is happening)
5. turn the payload as if it was standing up in a vertical rocket
6. You should hear [[#Beepcode Launched]]
## Magnitude IMU Detection Test
Configuration: 
- `EASY_BOOST_DETECT` is undefined
- `IMU_BOOST_DETECTION_MODE_AXIS` is undefined
Procedure
1. place the grim in the payload in flight mounting position
2. [[#Software Deploy|Deploy software]] and let it run
3. wait 10 seconds to make sure detection is not erroneously triggered by the altitude. (if this happens, call 781-812-8902 something very bad is happening)
4. Shake the payload (like a lot, it may be easier to shake only the board rather than the whole payload)
5. You should hear [[#Beepcode Launched]]
## Boost Detect Buffer Size Test

> [!CHECK] You might be chillin
> If you didn't edit any of `*_DATA_DELAY` these should be fine from their settings in Cleveland. If the [[#I2C lockup test]] didn't fail you don't need this. 

Run the [[#Axis IMU Detection Test]] or [[#Magnitude IMU Detection Test]] and follow the instructions for [[#After Landing]] to get the data off the device. In the output of the analysis script there will be a recomendation about what to set `ACCEL_BUFFER_SIZE` to. 


# Checklists and Procedures
## Software Deploy
- [ ] Take the modified `bme280.c` from `app_grim_reefer/extra/` and add to your zephyr tree at `$WORKSPACE/zephyr/drivers/sensor/bme280/` [^1]
- [ ] Ensure you have the correct flight parameters:
	- [ ] `#define BUZZER_USE_LED` is commented out
	- [ ] `#define SHORT_FLIGHT` is commented out
	- [ ] `#define EASY_BOOST_DETECT` is commented out
	- [ ] If using axis based IMU detection, ensure`#define IMU_UP_AXIS` is correct based on actual payload geometry
	- [ ] ensure `IMU_BOOST_DETECTION_MODE_AXIS` is defined if you want single axis detection. If magnitude is wanted, comment it out
	- [ ] `ADC_DATA_DELAY`, `ALTIM_DATA_DELAY`, and `FAST_DATA_DELAY` have values validated by the [[#i2c lockup test]]
	- [ ] `TOTAL_FLIGHT_TIME` is `K_SECONDS(400)` (probably shouldnt change, ask brax or donovan)
	- [ ] `CAMERA_EXTRA_TIME` is `K_MINUTES(12)` (probably shouldn't change, ask donathon or jean)
	- [ ] `ACCEL_VAL_THRESHOLD` is 5G `(9.81 * 5)`. shouldn't change
- [ ] Build the 'release' version of the software
```bash
cd FSW/FSW.git
make clean
make reefer_rel
west flash
```

On startup, if you have a serial connection to the board, it will print out some flight parameters to make sure you have the right ones selected.

[^1]: See [[#Modified BME280 Driver]]

## On the Pad

- [ ] Turn it on
- [ ] Wait for [[#Beepcode Waiting]]]
	- [ ] If you hear [[#Beepcode Launched]] grim has detected boost. power cycle
	- [ ] If you hear [[#Beepcode Missing Sensors]], power cycle. If it continues decide to scrub or launch
	- [ ] If you hear [[#Beepcode Missing Flash]], decide to scrub or launch
	- [ ] If you hear [[#Beepcode Low Battery]] you should have charged your battery more
	- [ ] If you hear [[#Beepcode Launched]] you shook the rocket too much. Power cycle the board and be gentler next time
- [ ] Ping the slack that launch is happening at some point soon
## After Landing

- [ ] Place the Grim in a ***secure place where it won't experience any high-G events***
	- <mark style="background: #FF5582A6;">***IF IT DOES, YOU WILL LOSE FLIGHT DATA***</mark> 
- [ ] Connect the following to the Grim and your computer accordingly
	- [ ] Power to +/- terminals
	- [ ] USB-C cable
- [ ] Open minicom with the appropriate device (OS Dependent)
	- [ ] `minicom -b 115200 -D <path to USB>`
- [ ]  type `grim nogo` to disable boost detection. You should be able to shake the payload to your hearts content now without it it launching (wouldnt recommend it though)
- [ ] Setup minicom to capture. (`Ctrl-A + L`, specify a file name on your computer , enter)
- [ ] type `grim dump`
- [ ] make pleasant conversation while you wait. This may take a while (~25 minutes)
- [ ] press `Ctrl-A + L` and select close
- [ ] edit the file to remove any leading or trailing, non base64 data (there will be some from you entering the dump command and from the shell prompt at the end)
- [ ] make at least 3 dissimilar backups (slack, google drive, github, multiple computers) or more. 
- [ ] Run the analysis script located at `app_grim_reefer/extra/analysis.py` to get csvs out of the launch data

# Appendix
## Modified BME280 Driver
The modified `bme280.c` removes the unconditional 3ms wait for sensor data reads. Waiting is needed to insure proper settings are in place. However, all our settings are set at init and not touched for the rest of the flight. This being the case, it is OK to not wait on the sensor like the general case does must. 

## Buzzer Codes
### Beepcode: Missing Flash 
Condition: If there is a problem setting up the flash chip or the filesystem
Bitcode:`101010101010101 00000000000000000`
Waveform: `-_-_-_-_-_-_-_-_ ________________`
Description: 8 quick chirps followed by 2 seconds of silence
### Beepcode: Landed
Condition: If the flight is over and data is stored
Note: this will start beeping when data is done, not when the cameras turn off (12 minutes later)
Bitcode:`1100110011001100 00000000000000000`
Waveform: `--__--__--__--__________________`
Description: 4 Chirps followed by 2 seconds of silence
### Beepcode: Launched 
Condition: If boost has been detected
Bitcode:`1110001110001110 00000000000000000`
Waveform: `---___---___---_________________`
Description: 3 chirps followed by 2 seconds of silence
### Beepcode: Low Battery 
Condition: If the battery is below the predetermined 'bad' level
Bitcode:`1111000011110000 00000000000000000`
Waveform: `----____----____________________`
Description: 2 beeps followed by 2 seconds of silence
### Beepcode: Missing Sensors 
Condition: If any sensors failed to initialize. This is a sign of the i2c lockup issue
Bitcode:`1111111100000000 00000000000000000`
Waveform: `--------________________________`
Description: 1 second beep followed by 3 seconds of silence
### Beepcode: Waiting
Condition: The board is ready and detecting launch
Bitcode:`101010101010101 00000000000000000`
Waveform: `-_______________________`
Description: 1 quick beep followed by just under 4 seconds of silence
