from typing import TypeAlias, List, Tuple, Set, Dict
import re
from collections import namedtuple
import sys
import csv
import argparse
from math import isnan
import magnetic_field


'''
Openrocket format that we care about:
Comments with name of sim
Comment with flight warnings
Empty Comment
Comment with header

Data intersperced with comment with event
'''


OREvent = namedtuple("OREvent", ['time', 'event'])
Reason: TypeAlias = str
Variable: TypeAlias = str
MissingVariable: TypeAlias = Tuple[Variable, Reason]
WantedVariables: TypeAlias = Tuple[Set[Variable], Reason]

StringPacket: TypeAlias = List[str]
Packet: TypeAlias = List[float]

event_finder = r"# Event ([a-zA-Z_]*) occurred at t=([+-]?([0-9]*[.])?[0-9]+) seconds"
recognized_events = ["IGNITION", "LAUNCH", "LIFTOFF", "LAUNCHROD", "BURNOUT", "EJECTION_CHARGE",
                     "APOGEE", "RECOVERY_DEVICE_DEPLOYMENT", "RECOVERY_DEVICE_DEPLOYMENT", "GROUND_HIT", "SIMULATION_END"]


def eprint(*args, **kwargs):
    RED_COLOR = '\033[91m'
    RESET_COLOR = '\033[0m'
    # helper to print to stderr
    print(RED_COLOR, end='', file=sys.stderr)
    print(*args, file=sys.stderr, flush=True, **kwargs)
    print(RESET_COLOR, end='', file=sys.stderr)


def load_file(filename: str) -> str:
    # load a file to string or fail with an error message
    try:
        with open(filename, 'r') as f:
            return f.read()
    except Exception as e:
        eprint(f"Couldn't load openrocket CSV file: {filename}")
        eprint(e)
        sys.exit(1)


def find_events(data_txt: str) -> List[OREvent]:
    '''
    Extract all events (encoded in comments) from the csv and convert them to our internal representation
    Events look like:
    # Event IGNITION occurred at t=0 seconds
    '''
    event_matches = re.findall(event_finder, data_txt)
    events: List[OREvent] = []
    for match in event_matches:
        event = match[0]
        time = match[1]

        if event not in recognized_events:
            eprint(f"WARNING: Unrecognized openrocket event {event}")
            continue

        events.append(OREvent(float(time), event))
    return events


def find_header(lines: List[str]) -> List[Variable]:
    '''
    The header is stored as a comment in (almost) the top of the file
    It tells us what data is in what column of the CSV
    '''
    header_line_idx = 0
    for linenum, line in enumerate(lines):
        if line == '#':
            header_line_idx = linenum+1
            break
    header = lines[header_line_idx][2:].split(",")
    return header


def read_data(lines: List[str]) -> List[StringPacket]:
    '''
    Read the csv and take its data
    Pythons csv reader does not support comments, luckily OR only puts them
    starting at the front of a line so theyre really easy to parse out.
    '''
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
    parser.add_argument('-m', '--magnetometer', action='store_true',
                        help="Should we expect and output Magnetometer data (lattitude, longitude, altitude)")

    return parser.parse_args()


def complain(missing_variables: List[MissingVariable]):
    # Complain about when you ask for more data than what is in your csv
    eprint(
        "Missing variables (you probably need to add these variable to your openrocket export:")
    eprint("Also: double check the units. We only support the ones listed below")
    missing = "Missing"
    reason = "Reason"

    eprint(f"{missing:40}\t{reason}")
    for (var, reason) in missing_variables:
        eprint(f"{var:40}\t{reason}")
    sys.exit(1)


def validate_vars(header: List[str], wanted: List[WantedVariables]) -> Dict[Variable, int]:
    # get a mapping of string to column name from the wanted variables
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
LONGITUDE = "Longitude (°)"
VELOCITY = "Total velocity (m/s)"
ALTITUDE = "Altitude (m)"
LATERAL_DIRECTION = "Lateral direction (°)"

VERT_ORIENTATION = "Vertical orientation (zenith) (°)"
LAT_ORIENTATION = "Lateral orientation (azimuth) (°)"

MAGNX = "Magnetometer X (gauss)"
MAGNY = "Magnetometer Y (gauss)"
MAGNZ = "Magnetometer Z (gauss)"

# order in the c struct. this will have to update as time goes on
struct_order = [TIME, VERT_ACCEL, LAT_ACCEL, ROLL, PITCH, YAW,
                TEMP, PRESSURE, LATITUDE, LONGITUDE, VELOCITY, ALTITUDE, LATERAL_DIRECTION, MAGNX, MAGNY, MAGNZ]


def convert_value(value: str) -> float:
    if isnan(float(value)):
        return 0
    return float(value)


def make_single_packet(packet, indices, mapping: Dict[Variable, int], config) -> List[float]:
    direct = [convert_value(packet[i]) for i in indices]

    # position
    calculated = []

    if config.magnetometer:
        lat = packet[mapping[LATITUDE]]
        long = packet[mapping[LONGITUDE]]
        alt = packet[mapping[ALTITUDE]]

        # orientation
        azimuth = packet[mapping[VERT_ORIENTATION]]
        inclination = packet[mapping[LAT_ORIENTATION]]

        magn = magnetic_field.evaluate_xyz(
            lat, long, alt, azimuth, inclination)

        calculated += [magn[0], magn[1], magn[2]]
    return direct+calculated


def collect_data(data: List[StringPacket], mapping: Dict[Variable, int], config) -> List[Packet]:
    indices = [mapping[key] for key in struct_order if key in mapping]

    wanted_data = [make_single_packet(
        packet, indices, mapping, config) for packet in data]

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


def make_c_file(filename: str, events: List[OREvent], data: List[Packet]):
    c_file = f'''#include "openrocket_sensors.h"

// generated from {{filename}}

#define NUM_DATA_PACKETS {len(data)}
const unsigned int or_packets_size = NUM_DATA_PACKETS;

#ifdef CONFIG_OPENROCKET_EVENT_LOG
#define NUM_EVENTS {len(events)}
const unsigned int or_events_size = NUM_EVENTS;
struct or_event_occurance_t or_events_data[NUM_EVENTS] = {{
{make_event_initializer(events)}
}};
const struct or_event_occurance_t * const or_events = or_events_data;

#endif

struct or_data_t or_packets_data[NUM_DATA_PACKETS] = {{

{make_initializer(data)}
}};

const struct or_data_t * const or_packets = or_packets_data;

'''
    return c_file


def get_wanted_vars(config) -> List[Variable]:
    # based on what sensors are enabled, what variables from the sim do we need
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
    if config.gnss:
        # https://docs.zephyrproject.org/latest/hardware/peripherals/gnss.html#c.navigation_data
        wanted_variables.append((
            {LATITUDE, LONGITUDE, VELOCITY, ALTITUDE, LATERAL_DIRECTION},
            "Requested GNSS data"))
    if config.magnetometer:
        wanted_variables.append((
            {VERT_ORIENTATION, LAT_ORIENTATION, LATITUDE, LONGITUDE, ALTITUDE},
            "Requested Magnetometer data"))
    return wanted_variables


def main():
    config = get_args()

    wanted_variables = get_wanted_vars(config)

    txt = load_file(config.in_filename)
    events = find_events(txt)
    lines = txt.splitlines()

    header = find_header(lines)
    mapping = validate_vars(header, wanted_variables)

    data = read_data(lines)
    filtered_data = collect_data(data, mapping, config)
    c_file = make_c_file(config.in_filename, events, filtered_data)

    try:
        with open(config.out_filename, 'w') as f:
            f.write(c_file)
    except Exception as e:
        eprint(f"Couldn't save file `{config.out_filename}:\n{e}`")
        sys.exit(1)


if __name__ == '__main__':
    main()
