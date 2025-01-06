import argparse
import os
import sys
import subprocess
import threading
from threading import Thread, Barrier


def generate_output_folder(args: argparse.Namespace, binary_fnames: list):
    """Generate the output folder"""
    if not os.path.exists(args.output):
        os.mkdir(args.output)

def delete_temp_file_mount(args: argparse.Namespace):
    """Delete the temporary file mount"""
    if os.path.exists(f"{args.output}/flash_mount"):
        subprocess.run(["fusermount", "-u", f"{args.output}/flash_mount"])
        os.rmdir(f"{args.output}/flash_mount")


def validate_arguments(args: argparse.Namespace) -> bool:
    """Validate the arguments passed in"""
    if not args.executable and not args.build_folder:
        print("No executable or build folder provided")
        return False

    if args.executable and not os.path.exists(args.executable):
        print("Executable does not exist")
        return False

    if args.build_folder and not os.path.exists(args.build_folder):
        print("Build folder does not exist")
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
    # return [f"{args.build_folder}/{file}" for file in os.listdir(args.build_folder)]


def setup_sim(args: argparse.Namespace):
    """Set up the simulation"""
    if not validate_arguments(args):
        return None

    binaries, binary_fnames = get_binaries(args)
    print(binaries)

    delete_temp_file_mount(args)
    generate_output_folder(args, binary_fnames)

    return binaries, binary_fnames


def generate_binary_flags(binary: str, args: argparse.Namespace) -> list:
    """Generate the flags to run the binary"""
    flags_list = []

    if args.real_time:
        flags_list.append("-rt")
    else:
        flags_list.append("-no-rt")

    # Place in temporary storage. Outputs get copied into top level before killing the binary
    flags_list.append(f"-flash-mount={args.output}/flash_mount")
    flags_list.append(f"-flash={binary}.bin")

    return flags_list


def run_simulation(start_barrier: threading.Barrier, stop_barrier: threading.Barrier,
                   binary_path: str, args: argparse.Namespace, base_output_folder: str):
    """Run the simulation for the given binary"""
    binary_fname = get_filename(binary_path)
    output_folder = f"{base_output_folder}/{binary_fname}"
    flags = generate_binary_flags(binary_path, args)

    if not os.path.exists(f"{args.output}/{binary_fname}"):
        os.mkdir(f"{args.output}/{binary_fname}")

    start_barrier.wait()

    with open(f"{output_folder}/{binary_fname}.log", "w") as log_file:
        process = subprocess.Popen([binary_path] + flags, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        for line in process.stdout:
            print(line.strip())
            log_file.write(line)
            if "RTOS Stopped!" in line:
                break

        # TODO: Returns -1 which is bad, but the files seem to be copied over properly
        # Might be some weird 64 vs 32 bit architecture issue too
        subprocess.run(["cp", "-r", f"{output_folder}/flash_mount/", f"{output_folder}/fs"])

        process.kill()
    stop_barrier.wait()


def cleanup(args: argparse.Namespace):
    """Cleanup the simulation"""
    delete_temp_file_mount(args)


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
    # TODO: Handle compiling (and dealing with sanitizers)

    args = parser.parse_args()

    binaries, binary_fnames = setup_sim(args)
    if not binaries:
        return

    print(f"Binaries to run: {binaries}")
    start_barrier = Barrier(len(binaries) + 1)
    stop_barrier = Barrier(len(binaries) + 1)

    for i, binary in enumerate(binaries):
        Thread(target=run_simulation, args=(start_barrier, stop_barrier,
                                            binary, args, args.output)).start()

    start_barrier.wait()
    print("Simulation started")

    stop_barrier.wait()
    print("Simulation complete")


if __name__ == "__main__":
    main()
