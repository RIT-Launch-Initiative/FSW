import argparse
from datetime import datetime

import yaml
import jinja2
import os

def main():
    template_path = __file__.split("ac_types.py")[0] + "templates/ac_network_defs.h"
    template = jinja2.Template(open(template_path).read())

    parser = argparse.ArgumentParser(description='Generate C++ struct types from a list of types.')
    parser.add_argument("-f", "--files", nargs='+', help="Files to generate types from")
    parser.add_argument("-o", "--output", help="Output file to write to")

    args = parser.parse_args()

    files = [os.getcwd() + "/" + fname for fname in args.files]

    for file in args.files:
        with open(file, 'r') as stream:
            try:
                data = yaml.safe_load(stream)


            except yaml.YAMLError as exc:
                print(exc)

    with open(args.output, 'w') as f:
        f.write(template.render(files=files, types=types, date_time=datetime.now()))

if __name__ == '__main__':
    main()