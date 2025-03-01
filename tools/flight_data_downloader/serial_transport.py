from fdd_transport import FDDTransport
import serial

class SerialTransport(FDDTransport):
    __slots__ = ["__serial_port", "__baud_rate"]

    def __init__(self):
        super().__init__()
        self.__serial_port = None

    def set_serial_port(self, serial_port: str):
        self.__serial_port = serial_port

    def set_baud_rate(self, baud_rate: int):
        if self.__serial_port is None:
            print("Serial port not set")

        self.__serial_port.baudrate = baud_rate

    def _get_file(self, file: str) -> bytes:
        if self.__serial_port is None:
            print("Serial port not set")

        with serial.Serial(self.__serial_port, baudrate=self.__baud_rate) as ser:
            ser.timeout = 5 # 5 seconds

            # Request the data
            ser.write("GET {}\n".format(file).encode())

            # Read size of file (64 bits / 8 bytes)
            file_size = int(ser.read(8).decode())
            if file_size == 0:
                print("File not found")
                return b""

            # Read bytes from serial
            return ser.read(file_size)
