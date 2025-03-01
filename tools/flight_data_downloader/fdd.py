import signal

from flight_data_downloader.fdd_transport import FDDTransport

transport: FDDTransport = None

def handle_set_command(args):
    pass

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