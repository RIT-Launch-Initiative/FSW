from typing import List, Tuple
import sys
from collections import namedtuple
import json
from jsonschema import validate, Draft202012Validator
import jsonschema.exceptions
import hashlib

filter = namedtuple("filter", ["state_transition", "state_to_measurement", "initial_state", "process_covariance", "measurement_covariance"])

help_str = '''
Lookup table and filter to C generator
Takes two quantile look up table paths and spits out C definitions to stdout

Usage:
    lut_to_c.py path/to/lut.json > include_me.h

'''


def generate_h(name: str, date_str: str, md5sum: bytes, flightTimeMs: int, lockoutMs: int, atmosphere: (float, float, List[float]), filter: filter, orientation_quat: List[float], xMin: float, xMax: float, lower_bounds: List[float], upper_bounds: List[float]) -> str:
    comma_separate_floats = lambda lst : ', '.join([str(v) for v in lst])
    orientation_quat_conjugate = [orientation_quat[0], -orientation_quat[1], -orientation_quat[2], -orientation_quat[3]]
    return f'''

#define LUT_NAME "{name}"
#define LUT_CREATION_DATE "{date_str}"
    
#define LUT_LOWER_BOUNDS_INITIALIZER {comma_separate_floats(lower_bounds)}

#define LUT_UPPER_BOUNDS_INITIALIZER {comma_separate_floats(upper_bounds)}

#define LUT_SIZE {len(lower_bounds)}

#define LUT_MINIMUM_X {float(xMin)}f
#define LUT_MAXIMUM_X {float(xMax)}f


#define KALMAN_STATE_TRANSITION_INITIALIZER {comma_separate_floats(filter.state_transition)}
#define STATE_TO_MEASUREMENT_INITIALIZER {comma_separate_floats(filter.state_to_measurement)}

#define KALMAN_INITIAL_STATE_INITIALIZER {comma_separate_floats(filter.initial_state)}
#define KALMAN_INITIAL_STATE_PROCESS_COVARIANCE {comma_separate_floats(filter.process_covariance)}
#define KALMAN_INITIAL_STATE_MEASUREMENT_COVARIANCE {comma_separate_floats(filter.measurement_covariance)}
#define AUTOGEN_IMU_TO_ROCKET_QUAT_INITIALIZER {comma_separate_floats(orientation_quat)}
#define AUTOGEN_IMU_TO_ROCKET_QUAT_CONJUGATED_INITIALIZER {comma_separate_floats(orientation_quat_conjugate)}

#define AUTOGEN_ATMOSPHERE_LUT_LEN {len(atmosphere[2])}
#define AUTOGEN_ATMOSPHERE_LUT_MIN_X {atmosphere[0]}
#define AUTOGEN_ATMOSPHERE_LUT_MAX_X {atmosphere[1]}
#define AUTOGEN_ATMOSPHERE_LUT_Y {comma_separate_floats(atmosphere[2])}

#define AUTOGEN_LOCKOUT_MS {lockoutMs}
#define AUTOGEN_FLIGHT_TIME_MS {flightTimeMs}

#define LUT_MD5SUM_ARRAY_LEN {len(md5sum)}
#define LUT_MD5SUM_INITIALIZER {', '.join([hex(b) for b in md5sum])}
#define LUT_MD5SUM_STR "{md5sum.hex()}"

'''

schema = {
    "$schema": "https://json-schema.org/draft-04/schema#",
    "type": "object",
    "properties":{
        "name":{
            "type": "string"
        },
        "date": {
            "type": "string",
            "format": "date-time"
        },
        "flight_time_ms":{
            "type": "number"
        },
        "lockout_ms":{
            "type": "number"
        },
        "orientation_quat": {
            "type": "array",
            "items": {
                "type": "number"
            },
            "minItems" : 4,
            "maxItems" : 4
        },
        "filter": {
            "type": "object",
            "properties":{
                "state_transition":{
                    "type": "array",
                    "items": {
                        "type":"number"
                    },
                    "minItems": 16,
                    "maxItems": 16
                },
                "state_to_measurement": {
                    "type": "array",
                    "items": {
                        "type": "number"
                    },
                    "minItems": 8,
                    "maxItems": 8
                },
                "initial_state":{
                    "type":"array",
                    "items":{
                        "type":[
                            "number"
                        ]
                    },
                    "minItems": 4,
                    "maxItems": 4
                },
                "process_covariance": {
                    "type": "array",
                    "items":{
                        "type": "number"
                    },
                    "minItems": 16,
                    "maxItems": 16
                },
                "measurement_covariance":{
                    "type":"array",
                    "items":{
                        "type":[
                            "number"
                        ]
                    },
                    "minItems": 4,
                    "maxItems": 4
                }
            },
            "required":[
                        "state_transition",
                        "state_to_measurement",
                        "initial_state",
                        "process_covariance",
                        "measurement_covariance",
            ]
        },
        "atmosphere":{
            "type": "object",
            "properties": {
                "pressure":{
                    "type":"array",
                    "items":{
                        "type": "number"
                    },
                },
                "altitude":{
                    "type":"array",
                    "items":{
                        "type":"number"
                    },
                }
            }
            
        },
        "quantile_lut": {
            "type": "object",
            "properties":{
                "x":{
                    "type":"array",
                    "items":{
                        "type": [
                            "number"
                        ]
                    }
                },
                "lower_bounds":{
                    "type":"array",
                    "items":{
                        "type":"number"
                    }
                },
                "upper_bounds":{
                    "type":"array",
                    "items": {
                        "type": "number"
                    }
                }
            },
            "required":[
                "x",
                "lower_bounds",
                "upper_bounds"
            ]
        }
    },
    "required":[
        "name",
        "date",
        "flight_time_ms",
        "lockout_ms",
        "orientation_quat",
        "filter",
        "atmosphere",
        "quantile_lut"
    ]
}

def validate_json(instance):
    try:
        validate(instance=instance, schema=schema, format_checker=Draft202012Validator.FORMAT_CHECKER)
    except jsonschema.exceptions.ValidationError as err:
        print(f"Invalid JSON data for LUT and filter: {err.message}", file=sys.stderr)
        exit(1)




def md5(path_file):
	checksum = hashlib.md5()
	
	fd = open(path_file, "rb")
	while True:
		data = fd.read(4096)
		if len(data) == 0:
			break
		checksum.update(data)
	
	return checksum.digest()



def main():
    if len(sys.argv) != 2:
        print(help_str)
        sys.exit(1)
    lut_path = sys.argv[1]
    
    with open(lut_path, 'r') as f:
        filter_desc = json.load(f)
    validate_json(filter_desc)

    md5sum = md5(lut_path)

    x, lower, upper = filter_desc["quantile_lut"]["x"], filter_desc["quantile_lut"]["lower_bounds"], filter_desc["quantile_lut"]["upper_bounds"]
    name, date = filter_desc["name"], filter_desc["date"]

    # sanity check arrays LUT
    if not (len(x) == len(lower) == len(upper)):
        print("Quantile lut definitions need the same number of elements")
        exit(1)

    kalman = filter_desc["filter"]
    flightTimeMs = filter_desc["flight_time_ms"]
    lockoutMs = filter_desc["lockout_ms"]
    atmosphere = filter_desc["atmosphere"]
    atmo_press = atmosphere["pressure"][::-1]
    atmo_alt = atmosphere["altitude"][::-1]
    
    filter2 = filter(kalman["state_transition"],kalman["state_to_measurement"], kalman["initial_state"], kalman["process_covariance"],kalman["measurement_covariance"])
    orientation = filter_desc["orientation_quat"]

    xMin, xMax = min(x), max(x)

    print(generate_h(name, date, md5sum, flightTimeMs, lockoutMs, (atmo_press[0], atmo_press[-1], atmo_alt), filter2, orientation, xMin, xMax, lower, upper))

if __name__ == '__main__':
    main()
