import argparse
import os
import sys
import subprocess
import threading
import logging
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

def generate_output_folder(args: argparse.Namespace, binary_fnames: list):
    """Generate the output folder"""
    if not os.path.exists(args.output):
        os.mkdir(args.output)

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

def setup_sim(args: argparse.Namespace):
    """Set up the simulation"""
    if not validate_arguments(args):
        return None

    binaries, binary_fnames = get_binaries(args)
    logger.info(f"Detected binaries: {binaries}")

    generate_output_folder(args, binary_fnames)

    return binaries, binary_fnames

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

def run_simulation(start_barrier: threading.Barrier, stop_barrier: threading.Barrier,
                   binary_path: str, args: argparse.Namespace, base_output_folder: str):
    """Run the simulation for the given binary"""
    binary_fname = get_filename(binary_path)
    output_folder = f"{base_output_folder}/{binary_fname}"
    flags = generate_binary_flags(binary_path, output_folder, args)

    if not os.path.exists(f"{args.output}/{binary_fname}"):
        os.mkdir(f"{args.output}/{binary_fname}")

    logger.info(f"Starting simulation for {binary_fname}")
    start_barrier.wait()

    with open(f"{output_folder}/{binary_fname}.log", "w") as log_file:
        process = subprocess.Popen([binary_path] + flags, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        for line in process.stdout:
            logger.debug(f"{binary_fname}: {line.strip()}")
            log_file.write(line)
            if "RTOS Stopped!" in line:
                logger.info(f"{binary_fname} finished running!")
                break

        # TODO: Returns -1 which is bad, but the files seem to be copied over properly
        # Might be some weird 64 vs 32 bit architecture issue too
        subprocess.run(["cp", "-r", f"{output_folder}/flash_mount/", f"{output_folder}/fs"])
        logger.info(f"Copied files from flash_mount to fs for {binary_fname}")

        process.kill()

    logger.info(f"Completed simulation for {binary_fname}")
    stop_barrier.wait()

def cleanup(args: argparse.Namespace):
    """Cleanup the simulation"""
    pass

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

    binaries, binary_fnames = setup_sim(args)
    if not binaries:
        logger.error("No binaries to run. Exiting.")
        return

    logger.info(f"Starting simulation with binaries: {binaries}")
    start_barrier = Barrier(len(binaries) + 1)
    stop_barrier = Barrier(len(binaries) + 1)

    for i, binary in enumerate(binaries):
        Thread(target=run_simulation, args=(start_barrier, stop_barrier, binary, args, args.output)).start()

    start_barrier.wait()
    logger.info("Simulation started")

    stop_barrier.wait()
    logger.info("Simulation complete")

if __name__ == "__main__":
    main()
