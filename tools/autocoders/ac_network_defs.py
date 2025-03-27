import argparse
from datetime import datetime
import yaml
import jinja2
import os

def parse_yaml_files(file_paths):
    general = []
    modules = dict()

    for file_path in file_paths:
        with open(file_path, 'r') as stream:
            try:
                data = yaml.safe_load(stream)
                general = data.get("general", [])
                modules = data.get("modules", {})
            except yaml.YAMLError as exc:
                print(f"Error parsing {file_path}: {exc}")

    return general, modules

if __name__ == '__main__':
    template_path = __file__.split(os.path.basename(__file__))[0] + "templates/ac_network_defs.h"
    template = jinja2.Template(open(template_path).read())

    parser = argparse.ArgumentParser(description='Generate Backplane network definitions.')
    parser.add_argument("-f", "--files", nargs='+', help="Files to generate network definitions from")
    parser.add_argument("-o", "--output", help="Output file to write to")

    args = parser.parse_args()

    file_paths = [os.path.join(os.getcwd(), fname) for fname in args.files]

    general, modules = parse_yaml_files(file_paths)

    with open(args.output, 'w') as f:
        f.write(template.render(files=file_paths, general=general, modules=modules, date_time=datetime.now()))
