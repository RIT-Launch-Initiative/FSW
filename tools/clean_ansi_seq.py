import re
import sys

def clean_ansi_sequences(input_file, output_file):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    uart_prompt = re.compile(r'^uart:~\$ ?')

    with open(input_file, 'r', encoding='utf-8') as infile, open(output_file, 'w', encoding='utf-8') as outfile:
        for line in infile:
            line = ansi_escape.sub('', line)
            line = uart_prompt.sub('', line)
            outfile.write(line)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python clean_log.py <input_file> <output_file>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    clean_ansi_sequences(input_path, output_path)
