from fdd_transport import FDDTransport
import tftpy
import io

from print_colors import print_red, print_green

RED_ASCII = "\033[91m"

def print_tftp_help():
    print("TFTP commands:")
    print("\tip <ip_address> - Set the TFTP server IP address")

class TFTPTransport(FDDTransport):
    __slots__ = ["__client"]

    def __init__(self):
        super().__init__()
        self.__client = None

    def set_ip(self, ip: str):
        self.__client = tftpy.TftpClient(ip, 69)

    def _get_file(self, file: str) -> bytes:
        if self.__client is None:
            print_red("IP address not set")
            return None

        buffer = io.BytesIO()
        self.__client.download(file, buffer)

        return buffer.getvalue()

    def set_attribute(self, attribute: str, args: list):
        if attribute == "ip":
            self.set_ip(args[0])
            print_green("TFTP server IP set to {}".format(args[0]))
        else:
            print_red("Invalid argument(s).")

    def __str__(self):
        return "tftp"