RED_ASCII = "\033[91m"
GREEN_ASCII = "\033[92m"

def print_red(text):
    print(RED_ASCII + text + "\033[0m")

def print_green(text):
    print(GREEN_ASCII + text + "\033[0m")