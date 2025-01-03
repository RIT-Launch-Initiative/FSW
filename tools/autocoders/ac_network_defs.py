import argparse
from datetime import datetime

import yaml
import jinja2
import os

def main():
    template_path = __file__.split(os.path.basename(__file__))[0] + "templates/ac_network_defs.h"
    template = jinja2.Template(open(template_path).read())

    parser = argparse.ArgumentParser(description='Generate Backplane network definitions.')
    parser.add_argument("-f", "--files", nargs='+', help="Files to generate network definitions from")
    parser.add_argument("-o", "--output", help="Output file to write to")

    args = parser.parse_args()

    files = [os.getcwd() + "/" + fname for fname in args.files]
    general = []
    modules = dict()
    for file in args.files:
        with open(file, 'r') as stream:
            try:
                data = yaml.safe_load(stream)
                general = data["general"]
                modules = data["modules"]

            except yaml.YAMLError as exc:
                print(exc)


    with open(args.output, 'w') as f:
        f.write(template.render(files=files, general=general, modules=modules, date_time=datetime.now()))

if __name__ == '__main__':
    main()