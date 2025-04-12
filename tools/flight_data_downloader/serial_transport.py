from fdd_transport import FDDTransport
from print_colors import print_red, print_green
import serial

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
        if self.__serial_port is None:
            print_red("Serial port not set")

        self.__baud_rate = baud_rate

    def _get_file(self, file: str) -> bytes:
        if self.__serial_port is None:
            print_red("Serial port not set")

        with serial.Serial(self.__serial_port, baudrate=self.__baud_rate) as ser:
            ser.timeout = 5  # 5 seconds

            # Request the data
            ser.write("GET {}\n".format(file).encode())

            # Read size of file (64 bits / 8 bytes)
            file_size = int(ser.read(8).decode())
            if file_size == 0:
                return None

            # Read bytes from serial
            return ser.read(file_size)

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