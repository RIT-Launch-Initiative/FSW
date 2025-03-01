import signal

from flight_data_downloader.fdd_transport import FDDTransport
from flight_data_downloader.serial_transport import SerialTransport
from flight_data_downloader.tftp_transport import TFTPTransport

transport = None

def handle_set_command_serial(args):
    if args[0] == "serial_port":
        transport.set_serial_port(args[1])
    elif args[0] == "baud_rate":
        transport.set_baud_rate(int(args[1]))

def handle_set_command_tftp(args):
    if args[0] == "ip":
        transport.set_ip(args[1])


def handle_set_command(args):
    global transport

    # Set the transport
    if args[0] == "transport":
        if args[1] == "tftp":
            transport = TFTPTransport()
        elif args[1] == "serial":
            transport = SerialTransport()
        else:
            print("Invalid transport")
        return

    # Set the output folder
    if args[0] == "output":
        if len(args) == 1:
            transport.set_output_folder(args[1])
        elif len(args) == 0: # No folder specified
            print("Output folder not specified")
        else: # Spaces in folder name
            print("Invalid output folder")

    # Applies to specific transports
    if transport is not None:
        if isinstance(transport, TFTPTransport):
            handle_set_command_tftp(args)
        elif isinstance(transport, SerialTransport):
            handle_set_command_serial(args)
    else:
        print("Transport not set")

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


def signal_handler(sig, frame):
    if sig == signal.SIGINT:
        handle_exit_command([])


def main():
    signal.signal(signal.SIGINT, signal_handler)

    while True:
        print(">> ", end="")
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
            case "exit":
                handle_exit_command(arguments)
            case "":
                continue
            case _:
                print("Invalid command")


if __name__ == '__main__':
    main()
