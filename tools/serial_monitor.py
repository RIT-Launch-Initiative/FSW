import serial
import argparse

def read_serial(port, baudrate, output_file):
    try:
        with serial.Serial(port, baudrate, timeout=1) as ser, open(output_file, 'a') as f:
            print(f"Connected to {port} at {baudrate} baud.")
            while True:
                try:
                    line = ser.readline().decode('utf-8', errors='replace').strip()
                    if line:
                        print(line)
                        f.write(line + '\n')
                        f.flush()
                except KeyboardInterrupt:
                    print("\nStopped by user.")
                    break
                except Exception as e:
                    print(f"Error reading line: {e}")
    except serial.SerialException as e:
        print(f"Failed to open serial port: {e}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Read from a serial port and log output to a file.')
    parser.add_argument('port', help='Serial port, e.g. /dev/ttyUSB0')
    parser.add_argument('baudrate', type=int, help='Baud rate, e.g. 115200')
    parser.add_argument('output_file', help='Output log file, e.g. mylog.txt')

    args = parser.parse_args()
    read_serial(args.port, args.baudrate, args.output_file)
