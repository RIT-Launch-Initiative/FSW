from fdd_transport import FDDTransport
from print_colors import print_red, print_green
import serial
import base64
from math import ceil

def print_serial_help():
    print("Serial commands:")
    print("\tport <port> - Set the serial port")
    print("\tbaud <baud_rate> - Set the baud rate")

class SerialTransport(FDDTransport):
    __slots__ = ["__serial_port", "__baud_rate"]

    def __init__(self):
        super().__init__()
        self.__serial_port = None
        self.__baud_rate = 115200

    def set_serial_port(self, serial_port: str):
        self.__serial_port = serial_port

    def set_baud_rate(self, baud_rate: int):
        self.__baud_rate = baud_rate

    def _get_file(self, file: str) -> bytes:
        if self.__serial_port is None:
            print_red("Serial port not set")

        with serial.Serial(port = self.__serial_port, baudrate = self.__baud_rate) as ser:
            ser.timeout = 0.5  # seconds
            ser.write('shell echo off\n'.encode())
            _ = ser.readlines() # flush out shell echo before beginning
            if not file.startswith('/'):
                # zephyr files must start with leading slash
                file = '/' + file

            # Request the data
            ser.write(f"fdd read {file}\n".encode())

            ser.readline() # Skip blank newline

            # Line containing file size or error message
            sizeorerror = ser.readline().decode()
            if sizeorerror.startswith("Size: "):
                file_size = int(sizeorerror.strip().replace('Size: ', ''))
            else:
                print_red(f"Error Reading: {sizeorerror}")
                return None
            
            data = bytearray()
            while True:
                l = ser.readline()
                if 'uart:~$'.encode() in l or len(l) == 0:
                    break

                dec = base64.decodebytes(l.strip())
                data.extend(dec)
            if file_size != len(data):
                print_red(f"Error decoding expected length != actual length ({file_size}, {len(data)})")
                return None
            return data

    def set_attribute(self, attribute, args: list):
        if args[0] == "port":
            self.set_serial_port(args[1])
            print_green("Serial port set to {}".format(self.__serial_port))
        elif args[0] == "baud":
            self.set_baud_rate(int(args[1]))
            print_green("Serial port set to {}".format(self.__serial_port))
        else:
            print_red("Invalid argument(s).")

    def __str__(self):
        return "serial"
    


