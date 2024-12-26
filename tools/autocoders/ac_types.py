import argparse
import yaml
import jinja2

def main():
    template_path = __file__.split("ac_types.py")[0] + "templates/ac_types.h"
    template = jinja2.Template(open(template_path).read())

    parser = argparse.ArgumentParser(description='Generate C++ types from a list of types.')
    parser.add_argument("-f", "--files", nargs='+', help="Files to generate types from")

    args = parser.parse_args()

    for file in args.files:
        with open(file, 'r') as stream:
            try:
                data = yaml.safe_load(stream)
                for key, value in data.items():
                    print(f"{key}: {value}")
            except yaml.YAMLError as exc:
                print(exc)


if __name__ == '__main__':
    main()