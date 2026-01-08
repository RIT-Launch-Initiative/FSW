from typing import TypeAlias, List, Tuple, Set, Dict
import re
from collections import namedtuple
import sys
import csv
import argparse
from math import isnan
import magnetic_field
from math import pi

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


def find_ork_events(csv_source_text: str) -> List[OREvent]:
    '''
    Extract all events (encoded in comments) from the csv and convert them to our internal representation
    Events look like:
    # Event IGNITION occurred at t=0 seconds
    '''
    event_matches = re.findall(event_finder, csv_source_text)
    events: List[OREvent] = []
    for match in event_matches:
        event = match[0]
        time = match[1]

        if event not in recognized_events:
            eprint(f"WARNING: Unrecognized openrocket event {event}")
            continue

        events.append(OREvent(float(time), event))
    return events


# header_str
# go from 'Name (unit)` to tuple of (name, unit)
def split_ork_csv_header_element(header_str: str):
    splitter = header_str.rindex('(')
    name = header_str[:splitter].strip()
    unit = header_str[splitter+1:header_str.rindex(')')]
    return (name, unit)


def find_csv_header(lines: List[str]) -> List[Variable]:
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
    return [split_ork_csv_header_element(item) for item in header]


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
    missing = "Missing"
    reason = "Reason"

    eprint(f"{missing:40}\t{reason}")
    for (var, reason) in missing_variables:
        eprint(f"{var:40}\t{reason}")
    sys.exit(1)


MappingDef = namedtuple(
    "MappingDef", ['header_index', 'original_unit', 'destination_unit'])


def validate_vars(csv_header: List[str], wanted: List[WantedVariables]) -> Dict[Variable, MappingDef]:
    # get a mapping of string to column name from the wanted variables
    missin_params: List[MissingVariable] = []
    mapping: Dict[Variable, int] = {}

    header_names = [h[0] for h in csv_header]

    for (variables, reason) in wanted:
        for variable in variables:
            if variable in header_names:
                header_index = header_names.index(variable)
                # index, from_unit
                mapping[variable] = MappingDef(
                    header_index, csv_header[header_index][1], desired_units[variable])
            else:
                missin_params.append((variable, reason))

    if len(missin_params) != 0:
        complain(missin_params)
    return mapping


TIME = "Time"

VERT_ACCEL = "Vertical acceleration"
LAT_ACCEL = "Lateral acceleration"
ROLL_RATE = "Roll rate"
PITCH_RATE = "Pitch rate"
YAW_RATE = "Yaw rate"

TEMP = "Air temperature"
PRESSURE = "Air pressure"

LATITUDE = "Latitude"
LONGITUDE = "Longitude"
VELOCITY = "Total velocity"
ALTITUDE = "Altitude"
LATERAL_DIRECTION = "Lateral direction"

VERT_ORIENTATION = "Vertical orientation (zenith)"
LAT_ORIENTATION = "Lateral orientation (azimuth)"

MAGNX = "Magnetometer X"
MAGNY = "Magnetometer Y"
MAGNZ = "Magnetometer Z"


desired_units = {
    TIME: 's',
    VERT_ACCEL: 'm/s²',
    LAT_ACCEL: 'm/s²',
    ROLL_RATE: 'rad/s',
    PITCH_RATE: 'rad/s',
    YAW_RATE: 'rad/s',
    TEMP: '°C',
    PRESSURE: 'kPa',
    LATITUDE: '°',
    LONGITUDE: '°',
    VELOCITY: 'm/s',
    ALTITUDE: 'm',
    LATERAL_DIRECTION: '°',
    VERT_ORIENTATION: '°',
    LAT_ORIENTATION: '°',
    MAGNX: 'gauss',
    MAGNY: 'gauss',
    MAGNZ: 'gauss',
}

# conversions
scaling_conversions = {
    # Time
    's': {'min': 1/60},

    # Length
    'cm': {'km': 1/1000, 'ft': 3.28084, 'yd': 1.0936133, 'mi': 0.000621371192, 'nmi': 0.000539956803},

    # Area
    'm²': {'cm²': 10000, 'mm²': 1000000, 'in²': 1550.0031, 'ft²': 10.7639104},


    # Linear Velocity
    'm/s': {'km/h': 3.6, 'ft/s': 3.28084, 'mph': 2.23694, 'kt': 1.94384},

    # Linear Acceleration (uppercase G bc lowercase g is grams)
    'm/s²': {'ft/s²': .3048, 'G': 0.101971621},

    # Rotation
    '°': {'rad': 0.0174532925, 'arcmin': 60},

    # Rotational Velocity
    '°/s': {'rad/s': 0.0174532925, 'r/s': 1/360, 'rpm': 1/6},

    # Mass
    'g': {'kg': 1/1000, 'oz': 0.0352739619, 'lb': 0.00220462},


    # Moment of Inertia
    'kg·m²': {'kg·cm²':  0.0001, 'oz·in²': 54674.74983, 'lb·in²': 3417.1718982, 'lb·ft²': 23.730360404},

    'N': {'lbf': 1, 'kgf': 1},

    'mbar': {'bar': .001, 'atm': 0.000986923, 'mmHg': 0.750062, 'inHg': 0.02953, 'psi': 0.0145038, 'Pa': 100, 'kPa': 0.1},
}
to_base_units = {value: key for key,
                 values in scaling_conversions.items() for value in values}

complicated_conversions = {
    '°C': {'°F': lambda C: ((C*9/5) + 32), 'K': lambda C: C+273.15},
    '°F': {'°C': lambda F: (F-32)*5/9, 'K': lambda F: ((F-32)*5/9)-273.15},
    'K': {'°C': lambda K: K-273.15, '°F': lambda K: (K-273.15)*9/5 + 32},
}
handled_by_scaling = [item for sub in [[base]+list(others.keys())
                                       for base, others in scaling_conversions.items()] for item in sub]
handled_by_complicated = list(complicated_conversions.keys())


def convert_unit(value: float, from_unit: str, to_unit: str):
    if from_unit == to_unit:
        return value

    if from_unit in handled_by_complicated:
        converter = complicated_conversions[from_unit][to_unit]
        return converter(value)
    elif from_unit in handled_by_scaling:
        if from_unit not in scaling_conversions.keys():
            # from unit is not base(key) unit
            base_unit_str = to_base_units[from_unit]
            from_to_base = 1/scaling_conversions[base_unit_str][from_unit]
            base_to_to = scaling_conversions[base_unit_str][to_unit]
            return value * from_to_base * base_to_to
        else:
            # from unit is base(key) unit
            factor = scaling_conversions[from_unit][to_unit]
            return value * factor
    else:
        raise Exception(f"Unsupported unit conversion {from_unit} to {to_unit}")


# order in the c struct. this will have to update as time goes on
struct_order = [TIME, VERT_ACCEL, LAT_ACCEL, ROLL_RATE, PITCH_RATE, YAW_RATE,
                TEMP, PRESSURE, LATITUDE, LONGITUDE, VELOCITY, ALTITUDE, LATERAL_DIRECTION, MAGNX, MAGNY, MAGNZ]


def denan_value(value: str) -> float:
    if isnan(float(value)):
        return 0
    return float(value)


def make_single_packet(packet, structFieldMappings: List[MappingDef], allMappings: Dict[Variable, MappingDef], config) -> List[float]:
    direct = [denan_value(convert_unit(float(packet[mapper.header_index]), mapper.original_unit,
                          mapper.destination_unit)) for mapper in structFieldMappings]

    calculated = []

    if config.magnetometer:
        lat = packet[allMappings[LATITUDE].header_index]
        long = packet[allMappings[LONGITUDE].header_index]
        alt = packet[allMappings[ALTITUDE].header_index]

        # orientation
        azimuth = packet[allMappings[VERT_ORIENTATION].header_index]
        inclination = packet[allMappings[LAT_ORIENTATION].header_index]

        magn = magnetic_field.evaluate_xyz(
            lat, long, alt, azimuth, inclination)

        calculated += [magn[0], magn[1], magn[2]]
    return direct+calculated


def collect_data(data: List[StringPacket], mapping: Dict[Variable, MappingDef], config) -> List[Packet]:
    mappingDefs = [mapping[key] for key in struct_order if key in mapping]

    wanted_data = [make_single_packet(
        packet, mappingDefs, mapping, config) for packet in data]

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
            ({VERT_ACCEL, LAT_ACCEL, ROLL_RATE, PITCH_RATE, YAW_RATE},
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
    events = find_ork_events(txt)
    lines = txt.splitlines()

    header = find_csv_header(lines)
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
