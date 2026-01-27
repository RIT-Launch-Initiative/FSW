from typing import List, Tuple
import sys


help_str = '''
Lookup table to CSV generator
Takes two quantile look up table paths and spits out C definitions to stdout

Usage:
    lut_to_c.py path/to/lower_bounds.csv path/to/upper_bounds.csv

'''


def generate_h(xMin: float, xMax: float, lower_bounds: List[float], upper_bounds: List[float]) -> str:
    comma_separate_floats = lambda lst : ', '.join([str(v) for v in lst])
    return f'''

#define LUT_LOWER_BOUNDS_INITIALIZER {comma_separate_floats(lower_bounds)}

#define LUT_UPPER_BOUNDS_INITIALIZER {comma_separate_floats(upper_bounds)}

#define LUT_SIZE {len(lower_bounds)}

#define LUT_MINIMUM_X {xMin}f
#define LUT_MAXIMUM_X {xMax}f
'''


def get_data(lower_bounds_path: str, upper_bounds_path: str) -> Tuple[List[float], List[float], List[float]]:
    '''
    returns x_axis, lower_values, upper_values
    '''
    def parse_lines(lines: List[str]):
        x = []
        y = []
        for line in lines[1:]:
            parts = [s.strip() for s in line.split(',')]
            if len(parts) != 2:
                print(f"Skipping line '{line}' - too many parts")
            x.append(float(parts[0]))
            y.append(float(parts[1]))

        return x, y

    with open(lower_bounds_path, 'r') as fl, open(upper_bounds_path, 'r') as fu:
        lb = list(fl.readlines())
        lu = list(fu.readlines())
        x1, lower = parse_lines(lb)
        x2, upper = parse_lines(lu)

        # if the two X-axes are not the same, fundamental assumptions of the implementation don't work anymore
        if len(x1) != len(x2):
            print("X AXES ARE NOT THE SAME LENGTH. CANNOT GENERATE LUT", file=sys.stderr)
            sys.exit(1)

        for i, (el1, el2) in enumerate(zip(x1, x2)):
            if el1 != el2:
                # i + 2: files are 1 indexed and we skip first line since its the header
                print(f"X AXES DO NOT MATCH ON LINE {i+2}: {el1} != {el2}", file=sys.stderr)
                sys.exit(1)

        return x1, lower, upper


def main():
    if len(sys.argv) != 3:
        print(help_str)
        sys.exit(1)
    lower_path = sys.argv[1]
    upper_path = sys.argv[2]

    x, lower, upper = get_data(lower_path, upper_path)
    xMin, xMax = min(x), max(x)
    print(generate_h(xMin, xMax, lower, upper))

if __name__ == '__main__':
    main()
