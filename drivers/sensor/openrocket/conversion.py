from typing import List, Tuple, Set, Dict
import re
from collections import namedtuple
import sys
import csv
import argparse
from math import isnan


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


event_occurance_type = "struct or_event_occurance_t"
or_data_type = "struct or_data_t"
'''
Openrocket format that we care about:
Comments with name of sim
Comment with flight warnings
Empty Comment
Comment with header

Data intersperced with comment with event
'''


def load_file(filename: str) -> str:
    try:
        with open(filename, 'r') as f:
            return f.read()
    except Exception as e:
        print(f"Error: Couldn't load openrocket CSV file: {filename}")
        print(e)
        sys.exit(1)


OREvent = namedtuple("OREvent", ['time', 'event'])
type Reason = str
type Variable = str
type MissingVariable = Tuple[Variable, Reason]
type WantedVariables = Tuple[Set[Variable], Reason]

type StringPacket = List[str]
type Packet = List[float]

event_finder = r"# Event ([a-zA-Z_]*) occurred at t=([+-]?([0-9]*[.])?[0-9]+) seconds"
recognized_events = ["IGNITION", "LAUNCH", "LIFTOFF", "LAUNCHROD", "BURNOUT", "EJECTION_CHARGE",
                     "APOGEE", "RECOVERY_DEVICE_DEPLOYMENT", "RECOVERY_DEVICE_DEPLOYMENT", "GROUND_HIT", "SIMULATION_END"]


def find_events(data_txt: str) -> List[OREvent]:
    event_matches = re.findall(event_finder, data_txt)
    events: List[OREvent] = []
    for match in event_matches:
        event = match[0]
        time = match[1]

        if event not in recognized_events:
            print(f"WARNING: Unrecognized openrocket event {event}")
            continue

        events.append(OREvent(float(time), event))
    return events


def find_header(lines: List[str]) -> List[Variable]:
    header_line_idx = 0
    for linenum, line in enumerate(lines):
        if line == '#':
            header_line_idx = linenum+1
            break
    header = lines[header_line_idx][2:].split(",")
    return header


def read_data(lines: List[str]) -> List[StringPacket]:
    nocomments = filter(lambda x: not x.startswith('#'), lines)
    reader = csv.reader(nocomments, delimiter=',', quotechar="'")
    return list(reader)


def get_args():
    parser = argparse.ArgumentParser(
        prog='converter.py',
        description='Converts Openrocket CSV exports into a C header file that simulated sensors in zephyr can use.',
        epilog="Use `converter.py -h` to show a help message")

    parser.add_argument(
        "in_filename", help="The filename of a csv file to convert from")
    parser.add_argument(
        "out_filename", help="The filename of the header file to output")
    parser.add_argument('-i', '--imu', action='store_true',
                        help="Should we expect and output date for an IMU (acceleration, roll, pitch, yaw)")
    parser.add_argument('-b', '--barometer', action='store_true',
                        help="Should we expect and output barometer data (temperature, pressure)")
    parser.add_argument('-g', '--gnss', action='store_true',
                        help="Should we expect and output GNSS data (lattitude, longitude, altitude)")

    return parser.parse_args()


def complain(missing_variables: List[MissingVariable]):
    eprint("Missing variables:")
    missing = "Missing"
    reason = "Reason"

    eprint(f"{missing:40}\t{reason}")
    for (var, reason) in missing_variables:
        eprint(f"{var:40}\t{reason}")
    sys.exit(1)


def validate_header(header: List[str], wanted: List[WantedVariables]) -> Dict[Variable, int]:
    missin_params: List[MissingVariable] = []
    mapping: Dict[Variable, int] = {}

    for (variables, reason) in wanted:
        for variable in variables:
            if variable in header:
                mapping[variable] = header.index(variable)
            else:
                missin_params.append((variable, reason))

    if len(missin_params) != 0:
        complain(missin_params)
    return mapping


TIME = "Time (s)"

VERT_ACCEL = "Vertical acceleration (m/s²)"
LAT_ACCEL = "Lateral acceleration (m/s²)"
ROLL = "Roll rate (°/s)"
PITCH = "Pitch rate (°/s)"
YAW = "Yaw rate (°/s)"

TEMP = "Air temperature (°C)"
PRESSURE = "Air pressure (mbar)"

LATITUDE = "Latitude (°)"
LONGITUDE = "Latitude (°)"
VELOCITY = "Total velocity (m/s)"
ALTITUDE = "Altitude (m)"

struct_order = [TIME, VERT_ACCEL, LAT_ACCEL, ROLL, PITCH, YAW,
                TEMP, PRESSURE, LATITUDE, LONGITUDE, VELOCITY, ALTITUDE]


def convert_value(value: str) -> float:
    if isnan(float(value)):
        return 0
    return float(value)


def filter_data(data: List[StringPacket], mapping: Dict[Variable, int]) -> List[Packet]:
    indices = [mapping[key] for key in struct_order if key in mapping]

    wanted_data = [[convert_value(packet[i])
                    for i in indices] for packet in data]
    return wanted_data


def format_packet(p: Packet) -> str:
    return '{' + (', '.join([f"{v: 4.4f}" for v in p])) + ' }'


def make_initializer(data: List[Packet]) -> str:
    s = ""
    for packet in data:
        s += format_packet(packet)+",\n"
    return s


def format_event(e: OREvent) -> str:
    return f"{{ {e.time}, OR_EVENT_{e.event} }}"


def make_event_initializer(events: List[OREvent]) -> str:
    s = ""
    for e in events:
        s += format_event(e) + ",\n"
    return s


def make_c_file(events: List[OREvent], data: List[Packet]):
    c_file = f'''#include "openrocket_imu.h"

#define NUM_EVENTS {len(events)}
#define NUM_DATA_PACKETS {len(data)}

const unsigned int or_events_size = NUM_EVENTS;
const unsigned int or_data_packets_size = NUM_DATA_PACKETS;

struct or_event_occurance_t[NUM_EVENTS] or_events = {{

{make_event_initializer(events)}
}};

struct or_data_t[NUM_DATA_PACKETS] or_packets = {{

{make_initializer(data)}
}};
'''
    return c_file


def main():
    config = get_args()

    wanted_variables = []
    wanted_variables.append(({TIME}, "Need Time for any sensor to work"))
    if config.imu:
        wanted_variables.append(
            ({VERT_ACCEL, LAT_ACCEL, ROLL, PITCH, YAW},
             "Requested IMU data"
             ))
    if config.barometer:
        wanted_variables.append(
            ({TEMP, PRESSURE},
             "Requested barometer data"))

        raise NotImplementedError("Barometer Output")
    if config.gnss:
        # https://docs.zephyrproject.org/latest/hardware/peripherals/gnss.html#c.navigation_data
        wanted_variables.append(({LATITUDE, LONGITUDE, VELOCITY, ALTITUDE},
                                 "Requested GNSS data"))
        raise NotImplementedError("GNSS Output")

    txt = load_file(config.in_filename)
    events = find_events(txt)
    lines = txt.splitlines()

    header = find_header(lines)
    mapping = validate_header(header, wanted_variables)

    data = read_data(lines)
    filtered_data = filter_data(data, mapping)

    c_file = make_c_file(events, filtered_data)

    try:
        with open(config.out_filename, 'w') as f:
            f.write(c_file)
    except Exception as e:
        eprint(f"Couldn't save file `{config.out_filename}:\n{e}`")
        sys.exit(1)


if __name__ == '__main__':
    main()
