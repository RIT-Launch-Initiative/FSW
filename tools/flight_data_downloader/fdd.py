import signal

from fdd_transport import FDDTransport
from serial_transport import SerialTransport, print_serial_help
from tftp_transport import TFTPTransport, print_tftp_help
from print_colors import print_red, print_green

transport = FDDTransport()

def handle_set_command(args):
    global transport

    attribute = args[0]
    args = args[1:]

    if attribute == "transport":
        if args[0] == "tftp":
            transport = TFTPTransport()
            print_green("Set to TFTP.")
        elif args[0] == "serial":
            transport = SerialTransport()
            print_green("Set to serial.")
        else:
            print_red("Invalid transport")
        return

    if attribute == "output":
        if len(args) == 1:
            transport.set_output_folder(args[0])
            print_green("Set output folder to {}".format(args[0]))
        elif len(args) == 0:  # No folder specified
            print_red("Output folder not specified")
        else:  # Spaces in folder name
            print_red("Invalid output folder")

        return

    if transport is not None and not isinstance(transport, FDDTransport):
        transport.set_attribute(attribute, args)
        return
    else:
        print_red("Transport not set")

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
    print("General commands:")
    print("\tset transport <tftp|serial> - Set the transport method")
    print("\tset output <folder> - Set the output folder")
    print("\ttree - Display the file tree")
    print("\tdownload <file> - Download a file")
    print("\tclear - Clear the screen")
    print("\texit - Exit the program")
    print("\thelp - Display this help message")

    print_tftp_help()
    print_serial_help()


def signal_handler(sig, frame):
    if sig == signal.SIGINT:
        handle_exit_command([])


def main():
    signal.signal(signal.SIGINT, signal_handler)

    print_help()

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
                print_red("Invalid command")


if __name__ == '__main__':
    main()
