import argparse
from datetime import datetime
import yaml
import jinja2
import os

def parse_yaml_types(file_paths):
    types_list = []

    for file_path in file_paths:
        with open(file_path, 'r') as stream:
            try:
                data = yaml.safe_load(stream)
                if data is None:
                    continue

                # 0th index of tuple is the name of the type
                # 1st index of tuple is key-value with the description and information for each field
                types_list += [(key, data[key]) for key in data]
            except yaml.YAMLError as exc:
                print(f"Error parsing YAML file: {file_path}")
                print(exc)
                exit(1)

    return types_list

if __name__ == '__main__':

    template_path = __file__.split(os.path.basename(__file__))[0] + "templates/ac_types.h"
    jinja_env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(os.path.dirname(template_path)),
        lstrip_blocks=True
    )
    template = jinja_env.get_template(os.path.basename(template_path))

    parser = argparse.ArgumentParser(description='Generate C++ struct types from a list of types.')
    parser.add_argument("-f", "--files", nargs='+', help="Files to generate types from")
    parser.add_argument("-o", "--output", help="Output file to write to")

    args = parser.parse_args()

    file_paths = [os.path.join(os.getcwd(), fname) for fname in args.files]

    types = parse_yaml_types(file_paths)

    with open(args.output, 'w') as f:
        f.write(template.render(files=file_paths, types=types, date_time=datetime.now()))
