from typing import List, Tuple
import sys
from collections import namedtuple
import json
from jsonschema import validate, Draft202012Validator
import jsonschema.exceptions
import hashlib

Controller = namedtuple("Controller", ["state_transition_matrix", "kalman_gain", "kalman_output", "initial_state"])

help_str = '''
Lookup table and controller to C generator
Takes two quantile look up table paths and spits out C definitions to stdout

Usage:
    lut_to_c.py path/to/lut.json > include_me.h

'''


def generate_h(name: str, date_str: str, md5sum: bytes, controller: Controller, orientation_quat: List[float], xMin: float, xMax: float, lower_bounds: List[float], upper_bounds: List[float]) -> str:
    comma_separate_floats = lambda lst : ', '.join([str(v) for v in lst])
    return f'''

#define LUT_NAME "{name}"
#define LUT_CREATION_DATE "{date_str}"
    
#define LUT_LOWER_BOUNDS_INITIALIZER {comma_separate_floats(lower_bounds)}

#define LUT_UPPER_BOUNDS_INITIALIZER {comma_separate_floats(upper_bounds)}

#define LUT_SIZE {len(lower_bounds)}

#define LUT_MINIMUM_X {float(xMin)}f
#define LUT_MAXIMUM_X {float(xMax)}f


#define KALMAN_STATE_TRANSITION_INITIALIZER {comma_separate_floats(controller.state_transition_matrix)}
#define KALMAN_OUTPUT_INITIALIZER {comma_separate_floats(controller.kalman_output)}
#define KALMAN_GAIN_INITIALIZER {comma_separate_floats(controller.kalman_gain)}
#define KALMAN_INITIAL_STATE_INITIALIZER {comma_separate_floats(controller.initial_state)}

#define KALMAN_UP_AXIS_QUAT_INITIALIZER {comma_separate_floats(orientation_quat)}

#define LUT_MD5SUM_ARRAY_LEN {len(md5sum)}
#define LUT_MD5SUM_INITIALIZER {', '.join([hex(b) for b in md5sum])}
#define LUT_MD5SUM_STR "{md5sum.hex()}"

'''



schema = {
    "$schema": "http://json-schema.org/draft-04/schema#",
    "type": "object",
    "properties": {
        "name": {
            "type": "string"
        },
        "date": {
            "type": "string",
            "format": "date-time"
        },
        "orientation_quat": {
            "type": "array",
            "items": {
                "type": "number"
            },
            "minItems": 4,
            "maxItems": 4
        },
        "controller": {
            "type": "object",
            "properties": {
                "state_transition_matrix": {
                "type": "array",
                "items": {
                    "type": "number"
                },
            "minItems": 16,
            "maxItems": 16

            },
            "kalman_gain": {
                "type": "array",
                "items": {
                    "type": "number"
                },
                "minItems": 8,
                "maxItems": 8

            },
            "kalman_output": {
                "type": "array",
                "items": {
                    "type": "number",
                },
                "minItems": 8,
                "maxItems": 8

            },
            "initial_state": {
                "type": "array",
                "items": {
                    "type": [
                    "number"
                    ]
                },
                "minItems": 4,
                "maxItems": 4
            }
        },
        "required": [
            "state_transition_matrix",
            "kalman_gain",
            "kalman_output",
            "initial_state"
        ]
        },
        "quantile_lut": {
        "type": "object",
        "properties": {
            "x": {
            "type": "array",
            "items": {
                "type": [
                "number",
                ]
            }
            },
            "lower_bounds": {
            "type": "array",
            "items": {
                "type": "number"
            }
            },
            "upper_bounds": {
            "type": "array",
            "items": {
                "type": "number"
            }
            }
        },
        "required": [
            "x",
            "lower_bounds",
            "upper_bounds"
        ]
    }
  },
  "required": [
    "name",
    "date",
    "orientation_quat",
    "controller",
    "quantile_lut"
  ]
}


def validate_json(instance):
    try:
        validate(instance=instance, schema=schema, format_checker=Draft202012Validator.FORMAT_CHECKER)
    except jsonschema.exceptions.ValidationError as err:
        print(f"Invalid JSON data for LUT and controller: {err.message}", file=sys.stderr)
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
        controller_desc = json.load(f)
    validate_json(controller_desc)

    md5sum = md5(lut_path)

    x, lower, upper = controller_desc["quantile_lut"]["x"], controller_desc["quantile_lut"]["lower_bounds"], controller_desc["quantile_lut"]["upper_bounds"]
    name, date = controller_desc["name"], controller_desc["date"]

    # sanity check arrays LUT
    if not (len(x) == len(lower) == len(upper)):
         print("Quantile lut definitions need the same number of elements")
         exit(1)

    kalman = controller_desc["controller"]
    controller = Controller(kalman["state_transition_matrix"], kalman["kalman_gain"], kalman["kalman_output"], kalman["initial_state"])
    orientation = controller_desc["orientation_quat"]

    xMin, xMax = min(x), max(x)

    print(generate_h(name, date, md5sum, controller, orientation, xMin, xMax, lower, upper))

if __name__ == '__main__':
    main()
