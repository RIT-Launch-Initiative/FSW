# %% [markdown]
# # Data analysis script
#
# ## Input
# a single file received from running `grim dump` on the payload
#
# ## Output
# - FILL THIS IN

# %%
from functools import reduce
import struct
import base64
from collections import namedtuple
from typing import Tuple, List
import pandas as pd
import os


# %% [markdown]
# ## Data opening and parsing
# - open a file to bytes
# - bytes to lists of data

# %%
SEPARATOR = "********\n"
filename = 'data/all.cap'
ouptut_dir = 'out2'

# %%


def file_to_bytes(filename: str) -> Tuple[bytes, bytes, bytes, bytes, bytes]:
    with open(filename, 'r') as f:
        parts = f.read().split(SEPARATOR)
        if len(parts) != 5:
            print(
                "wrong number of parts. did you remove all extra lines on top and bottom")
        byte_parts = [base64.b64decode(part) for part in parts]
        return tuple(byte_parts)


# %%
SlowData = namedtuple('SlowData', ['timestamp', 'humidity', 'temperature', 'grim_voltage',
                      'grim_current', 'load_cell_voltage', 'load_cell_current', 'bat_voltage', 'bat_current'])
FastData = namedtuple('FastData', [
                      'timestamp', 'accx', 'accy', 'accz', 'gyrox', 'gyroy', 'gyroz', 'pressure'])
ADCData = namedtuple('ADCData', ['timestamp', 'reading'])
# PreIMUData is FastData
PreALTData = namedtuple('PreALTData', ['timestamp', 'pressure', 'temperature'])


# %%
slow_units = SlowData('ms', '% humidity', 'degrees C',
                      'mV', 'mA', 'mV', 'mA', 'mV', 'mA')
fast_units = FastData('ms', 'm/s^2', 'm/s^2', 'm/s^2',
                      'rad/s',  'rad/s',  'rad/s', 'kPa')
adc_units = ADCData('ms', 'LSB')
pre_altitude_units = PreALTData('ms', 'kPa', 'degrees C')

# %%
slow_fmt = 'IffHHHHHH'  # timestamp, humid, temp, (voltage, current) * 3
fast_fmt = 'Ifffffff'  # timestamp accxyz, gyro xyz, press
adc_fmt = 'Iiiiiiiiiii'  # timestamp + 10 int32s
imu_boost_detect_fmt = fast_fmt
alt_boost_detect_fmt = 'Iff'  # timestamp, press, temp

# %%
slow_bs, fast_bs, adc_bs, pre_imu_bs, pre_alt_bs = file_to_bytes(filename)

# %%


def interpolate_adc(entries: List[List]) -> List[ADCData]:
    l = []
    for i, entry in enumerate(entries[:-1]):
        start_time = entries[i][0]
        period = entries[i+1][0] - entries[i][0]
        per = period / 10.0
        for j, sample in enumerate(entry[1:]):
            l.append(ADCData(start_time + j * per, sample))
    return l


# %%
slow_lists = list(struct.iter_unpack(slow_fmt, slow_bs))
fast_lists = list(struct.iter_unpack(fast_fmt, fast_bs))
adc_lists = list(struct.iter_unpack(adc_fmt, adc_bs))
pre_imu_lists = list(struct.iter_unpack(imu_boost_detect_fmt, pre_imu_bs))
pre_alt_lists = list(struct.iter_unpack(alt_boost_detect_fmt, pre_alt_bs))


# %%
def unit_slow_data(l: List) -> SlowData:
    dr = SlowData(*l)
    du = SlowData(dr.timestamp, dr.humidity, dr.temperature, 1.25 * dr.grim_voltage, 1.25 * dr.grim_voltage,
                  1.25 * dr.load_cell_voltage, 1.25 * dr.load_cell_current, 1.25 * dr.bat_voltage, 1.25 * dr.bat_current)
    return du


# %%
slow_data = [unit_slow_data(l) for l in slow_lists]
fast_data = [FastData(*l) for l in fast_lists]
adc_data = interpolate_adc(adc_lists)

pre_imu_data_circ = [FastData(*l) for l in pre_imu_lists if l[0] != 0][:-14]
# filter out timestamp = 0 (unwritten) entries
# order by timestamp bc the circular buffer may not begin with the earliest entry
pre_imu_data = sorted(pre_imu_data_circ, key=lambda d: d.timestamp)
pre_alt_data = sorted(
    [PreALTData(*l) for l in pre_alt_lists if l[0] != 0], key=lambda d: d.timestamp)

# %%


# %%
slow_data_elapsed_s = (
    slow_data[-1].timestamp - slow_data[0].timestamp) / 1000.0
fast_data_elapsed_s = (
    fast_data[-1].timestamp - fast_data[0].timestamp) / 1000.0
adc_data_elapsed_s = (adc_data[-1].timestamp - adc_data[0].timestamp) / 1000.0

flight_elapsed_s = adc_data_elapsed_s

pre_alt_elapsed_ms = pre_alt_data[-1].timestamp - pre_alt_data[0].timestamp
pre_imu_elapsed_ms = pre_imu_data[-1].timestamp - pre_imu_data[0].timestamp

# %%
print(f"IMU Buffer Time: {pre_imu_elapsed_ms} ms")
print(f"ALT Buffer Time: {pre_alt_elapsed_ms} ms")
print(f"Flight Time: {flight_elapsed_s} s")

# %%
percent_imu = 250 / pre_imu_elapsed_ms
print(f"Suggest {percent_imu} of current IMU Buffer size")

# %%
# calculate average period of each reading thread
print(f"{len(slow_data)} slow entries.  avg {flight_elapsed_s / len(slow_data)} second period")
print(f"{len(fast_data)} fast entries.  avg {1000 * flight_elapsed_s / len(fast_data)} ms period")
print(f"{len(adc_data)} adc entries.  avg {1000 * flight_elapsed_s / len(adc_data)} ms period")


# %%
slow_df = pd.DataFrame(slow_data)
fast_df = pd.DataFrame(fast_data)
adc_df = pd.DataFrame(adc_data)

pre_imu_df = pd.DataFrame(pre_imu_data)
pre_alt_df = pd.DataFrame(pre_alt_data)


# %%
# set T=0 to start of boost accel buffer
start = pre_imu_df['timestamp'][0]

slow_df['timestamp'] = slow_df['timestamp'] - start
fast_df['timestamp'] = fast_df['timestamp'] - start
adc_df['timestamp'] = adc_df['timestamp'] - start

pre_imu_df['timestamp'] = pre_imu_df['timestamp'] - start
pre_alt_df['timestamp'] = pre_alt_df['timestamp'] - start

# %%


def add_units_to_df(df: pd.DataFrame, units):
    new_names = {}
    for unit, (series_name, _) in zip(units, df.items()):
        new_names[series_name] = f"{series_name} ({unit})"
    return df.rename(columns=new_names)


# %%
if not os.path.exists(ouptut_dir):
    os.makedirs(ouptut_dir)

add_units_to_df(slow_df, slow_units).to_csv(
    ouptut_dir+'/slow.csv', index=False)
add_units_to_df(fast_df, fast_units).to_csv(
    ouptut_dir+'/fast.csv', index=False)
add_units_to_df(adc_df, adc_units).to_csv(ouptut_dir+'/adc.csv', index=False)
add_units_to_df(pre_alt_df, pre_altitude_units).to_csv(
    ouptut_dir+'/pre_imu.csv', index=False)
add_units_to_df(pre_imu_df, fast_units).to_csv(
    ouptut_dir+'/pre_alt.csv', index=False)

# %%

all_dfs = [fast_df, slow_df, adc_df, pre_imu_df, pre_alt_df]

df_merged = reduce(lambda left, right: pd.merge(left, right, on=['timestamp'],
                                                how='outer'), all_dfs)

# %%
df_merged.interpolate(inplace=True, limit_direction='both')

# %%
df_merged

# %%
df_merged.sort_values('timestamp')

# %%
df_merged.plot('timestamp')

# %%
fast_df.plot('timestamp', title="Fast Sensors")
slow_df.plot('timestamp', title="Slow Sensors")
adc_df.plot('timestamp', title="ADC")

pre_imu_df.plot('timestamp', title="Boost Detect IMU")
pre_alt_df.plot('timestamp', title="Boost Detect Accel")

# %%


# %%


# %%
