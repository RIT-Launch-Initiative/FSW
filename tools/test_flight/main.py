import argparse
import os
import subprocess
from csv import excel
from sys import excepthook
import signal

from threading import Thread, Barrier


def delete_temp_file_mount(args):
    """Delete the temporary file mount"""
    if os.path.exists(f"{args.output}/flash_mount"):
        subprocess.run(["fusermount", "-u", f"{args.output}/flash_mount"])
        os.rmdir(f"{args.output}/flash_mount")


def validate_arguments(args) -> bool:
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


def get_binaries(args) -> list:
    """Get the list of binaries to run"""
    if args.executable:
        return [args.executable]
    return [f"{args.build_folder}/{file}" for file in os.listdir(args.build_folder)]


def setup_sim(args):
    """Set up the simulation"""
    if not validate_arguments(args):
        return None

    delete_temp_file_mount(args)


    return get_binaries(args)

def generate_binary_flags(binary, args) -> list:
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


def run_simulation(start_barrier, stop_barrier, binary_path, args, output_folder):
    """Run the simulation for the given binary"""
    binary = binary_path.split("/")[-1]
    flags = generate_binary_flags(binary_path, args)
    start_barrier.wait()

    with open(f"{output_folder}/{binary}.out", "w") as log_file:
        process = subprocess.Popen([binary_path] + flags, stdout=log_file, stderr=log_file)

        while True:
            line = process.stdout.readline()
            print(line.decode("utf-8").strip())
            if not line:
                break
            print(line.decode("utf-8").strip())
            if "RTOS Stopped!" in line.decode("utf-8"):
                print("RTOS Stopped!")
                break

        # Copy the flash mount to the top level
        subprocess.run(["cp", "-r", f"{output_folder}/flash_mount", output_folder])

        process.kill()
    stop_barrier.wait()


def cleanup(args):
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

    binaries = setup_sim(args)
    if not binaries:
        return

    print(f"Binaries to run: {binaries}")
    start_barrier = Barrier(len(binaries) + 1)
    stop_barrier = Barrier(len(binaries) + 1)

    for binary in binaries:
        Thread(target=run_simulation, args=(start_barrier, stop_barrier, binary, args, args.output)).start()

    start_barrier.wait()
    print("Simulation started")

    stop_barrier.wait()
    print("Simulation complete")


if __name__ == "__main__":
    main()
