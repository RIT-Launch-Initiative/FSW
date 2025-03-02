import signal

from fdd_transport import FDDTransport
from serial_transport import SerialTransport
from tftp_transport import TFTPTransport

transport = FDDTransport()

def handle_set_command(args):
    global transport

    attribute = args[0]
    args = args[1:]

    if attribute == "transport":
        if args[0] == "tftp":
            transport = TFTPTransport()
        elif args[0] == "serial":
            transport = SerialTransport()
        else:
            print("Invalid transport")
        return

    if attribute == "output":
        if len(args) == 1:
            transport.set_output_folder(args[0])
        elif len(args) == 0:  # No folder specified
            print("Output folder not specified")
        else:  # Spaces in folder name
            print("Invalid output folder")

        return

    if transport is not None:
        transport.set_attribute(attribute, args)
        return


def handle_tree_command():
    if transport is not None:
        transport.tree()


def handle_download_command(args):
    if transport is not None:
        for file in args:
            transport.download(file)


def handle_exit_command(args):
    exit(0)


def clear_screen():
    print("\033[H\033[J")


def print_help():
    print("Available commands:")
    print("\tset transport <tftp|serial> - Set the transport method")
    print("\tset output <folder> - Set the output folder")
    print("\tset ip <address> - Set the TFTP server IP address")
    print("\tset port <port> - Set the serial port")
    print("\tset baud <rate> - Set the serial baud rate")
    print("\ttree - Display the file tree")
    print("\tdownload <file> - Download a file")
    print("\tclear - Clear the screen")
    print("\texit - Exit the program")
    print("\thelp - Display this help message")


def signal_handler(sig, frame):
    if sig == signal.SIGINT:
        handle_exit_command([])


def main():
    signal.signal(signal.SIGINT, signal_handler)

    while True:
        print(str(transport) + ">> ", end="")
        user_input = input().lower()
        command = user_input.split(" ")[0]
        arguments = user_input.split(" ")[1:]

        match command:
            case "set":
                handle_set_command(arguments)
            case "tree":
                handle_tree_command()
            case "download":
                handle_download_command(arguments)
            case "clear":
                clear_screen()
            case "help":
                print_help()
            case "exit":
                handle_exit_command(arguments)
            case "":
                continue
            case _:
                print("Invalid command")


if __name__ == '__main__':
    main()
