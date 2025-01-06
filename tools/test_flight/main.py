import argparse
import os
import sys
import subprocess
import threading
import logging
from datetime import datetime
from threading import Thread, Barrier

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("simulation.log"),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger(__name__)

# Globals
WARNING_ERRORS_LOCK = threading.Lock()

WARNINGS_DICT = dict()
ERRORS_DICT = dict()

WARNINGS_COUNT = 0
ERRORS_COUNT = 0

SUPPRESSED_ERRORS = [
    "Corrupted dir pair at {0x0, 0x1}"  # Zephyr automount
]

SUPPRESSED_WARNINGS = [
    "Skipping bind. Using native_sim loopback",  # Launch Core Simulation uses socket offloading w/ loopback
    "can't mount (LFS -84); formatting"  # Zephyr automount
]


def generate_output_folder(args: argparse.Namespace) -> str:
    """Generate the output folder"""
    if not os.path.exists(args.output):
        os.mkdir(args.output)

    # Get timestamp
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    output_folder = f"{args.output}/{timestamp}"
    os.mkdir(output_folder)

    return output_folder


def validate_arguments(args: argparse.Namespace) -> bool:
    """Validate the arguments passed in"""
    if not args.executable and not args.build_folder:
        logger.error("No executable or build folder provided")
        return False

    if args.executable and not os.path.exists(args.executable):
        logger.error("Executable does not exist")
        return False

    if args.build_folder and not os.path.exists(args.build_folder):
        logger.error("Build folder does not exist")
        return False

    if not os.path.exists(args.output):
        os.mkdir(args.output)

    return True


def get_filename(file_path: str) -> str:
    """Get the filename of the binary"""
    return file_path.split("/")[-1]


def get_binaries(args: argparse.Namespace) -> tuple[list, list]:
    """Get the list of binaries to run"""
    if args.executable:
        return [args.executable], [get_filename(args.executable)]

    # Warning
    # TODO: Can't do multiple executables yet, until multiple network interfaces is set up
    sys.stderr.write("UNSUPPORTED: READ TODO IN CODE\n")
    return [], []


def setup_sim(args: argparse.Namespace) -> tuple[list, list, str]:
    """Set up the simulation"""
    if not validate_arguments(args):
        return None

    binaries, binary_fnames = get_binaries(args)
    logger.info(f"Detected binaries: {binaries}")

    output_folder = generate_output_folder(args)

    return binaries, binary_fnames, output_folder


def generate_binary_flags(binary: str, output_folder: str, args: argparse.Namespace) -> list:
    """Generate the flags to run the binary"""
    flags_list = []

    if args.real_time:
        flags_list.append("-rt")
    else:
        flags_list.append("-no-rt")

    # Place in temporary storage. Outputs get copied into top level before killing the binary
    flags_list.append(f"-flash-mount={output_folder}/flash_mount")
    flags_list.append(f"-flash={binary}.bin")

    return flags_list


def add_warnings_errors(binary_fname: str, error_lines: list, warning_lines: list) -> None:
    """Add the warnings and errors to the global dictionaries"""
    global WARNINGS_COUNT
    global ERRORS_COUNT

    with WARNING_ERRORS_LOCK:
        WARNINGS_DICT[binary_fname] = warning_lines
        ERRORS_DICT[binary_fname] = error_lines

        ERRORS_COUNT += len(error_lines)
        WARNINGS_COUNT += len(warning_lines)


def run_simulation(start_barrier: threading.Barrier, stop_barrier: threading.Barrier,
                   binary_path: str, args: argparse.Namespace, base_output_folder: str) -> None:
    """Run the simulation for the given binary"""
    binary_fname = get_filename(binary_path)
    output_folder = f"{base_output_folder}/{binary_fname}"
    flags = generate_binary_flags(binary_path, output_folder, args)

    if not os.path.exists(output_folder):
        os.mkdir(output_folder)

    logger.info(f"Starting simulation for {binary_fname}")
    start_barrier.wait()

    error_lines = []
    warning_lines = []

    with open(f"{output_folder}/{binary_fname}.log", "w") as log_file:
        process = subprocess.Popen([binary_path] + flags, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        for line in process.stdout:
            logger.debug(f"{binary_fname}: {line.strip()}")
            log_file.write(line)
            if "RTOS Stopped!" in line:
                logger.info(f"{binary_fname} finished running!")
                break

            if "err" in line and not any(error in line for error in SUPPRESSED_ERRORS):
                error_lines.append(line)

            if "wrn" in line and not any(warning in line for warning in SUPPRESSED_WARNINGS):
                warning_lines.append(line)
        add_warnings_errors(binary_fname, error_lines, warning_lines)
        # TODO: Returns -1 which is bad, but the files seem to be copied over properly
        # Might be some weird 64 vs 32 bit architecture issue too
        subprocess.run(["cp", "-r", f"{output_folder}/flash_mount/", f"{output_folder}/fs"])
        logger.info(f"Copied files from flash_mount to fs for {binary_fname}")

        process.kill()

    logger.info(f"Completed simulation for {binary_fname}")
    stop_barrier.wait()


def cleanup() -> None:
    """Cleanup the simulation"""
    pass


def print_all_warnings_errors() -> None:
    print("---- WARNINGS ----")
    if WARNINGS_COUNT != 0:
        for binary, warnings in WARNINGS_DICT.items():
            print(f"Warnings for {binary}:")
            for warning in warnings:
                print("\t", warning)
    else:
        print("No warnings found")

    print("---- ERRORS ----")
    if ERRORS_COUNT != 0:
        for binary, errors in ERRORS_DICT.items():
            print(f"Errors for {binary}:")
            for error in errors:
                print("\t", error)
    else:
        print("No errors found")


def print_results() -> None:
    print("#### RESULTS ####")
    print_all_warnings_errors()


def main():
    """Parse CLI args and run appropriate functions for simulation"""
    parser = argparse.ArgumentParser(description="RIT Launch Initiative's SIL tool for FSW")

    # NOTE: Either executable or build folder must be provided
    parser.add_argument("-e", "--executable", help="Executable to run in the simulation")
    parser.add_argument("-b", "--build_folder", help="Folder of executables to execute in the simulation")
    parser.add_argument("-o", "--output", help="Outputs folder", default="tf_out")

    # Extra Configuration Fun
    parser.add_argument("-rt", "--real-time", help="Run the simulation in real-time", action="store_true",
                        default=False)

    args = parser.parse_args()
    if not args.executable and not args.build_folder:
        parser.print_help()
        return

    binaries, binary_fnames, output_folder = setup_sim(args)
    if not binaries:
        logger.error("No binaries to run. Exiting.")
        return

    logger.info(f"Starting simulation with binaries: {binaries}")
    start_barrier = Barrier(len(binaries) + 1)
    stop_barrier = Barrier(len(binaries) + 1)

    for i, binary in enumerate(binaries):
        Thread(target=run_simulation, args=(start_barrier, stop_barrier, binary, args, output_folder)).start()

    start_barrier.wait()
    logger.info("Simulation started")

    stop_barrier.wait()
    logger.info("Simulation complete")

    print_results()


if __name__ == "__main__":
    main()
