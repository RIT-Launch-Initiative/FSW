from typing import List, Tuple
import sys


help_str = '''
Lookup table to CSV generator
Takes two quantile look up table paths and spits out C definitions to stdout

Usage:
    lut_to_c.py path/to/lut.csv > include_me.h

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


def get_data(lut_path: str) -> Tuple[List[float], List[float], List[float]]:
    '''
    returns x_axis, lower_values, upper_values
    '''
    def parse_lines(lines: List[str]):
        x = []
        yl = []
        yh = []
        for line in lines[1:]: # skip header
            parts = [s.strip() for s in line.split(',')]
            if len(parts) != 3:
                print(f"Skipping line '{line}' - wrong number of values (want 3)", file=sys.stderr)
            x.append(float(parts[0]))
            yl.append(float(parts[1]))
            yh.append(float(parts[2]))

        return x, yl, yh

    with open(lut_path, 'r') as f:
        lines = list(f.readlines())
        x, lower, upper = parse_lines(lines)

        return x, lower, upper


def main():
    if len(sys.argv) != 2:
        print(help_str)
        sys.exit(1)
    lut_path = sys.argv[1]
    
    x, lower, upper = get_data(lut_path)
    xMin, xMax = min(x), max(x)
    print(generate_h(xMin, xMax, lower, upper))

if __name__ == '__main__':
    main()
