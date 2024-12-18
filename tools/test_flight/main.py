import argparse
import os
import subprocess

from threading import Thread, Barrier


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

    if not os.path.exists(f"{args.output}/flash_mount"):
        os.mkdir(f"{args.output}/flash_mount")

    return True

def get_binaries(args) -> list:
    """Get the list of binaries to run"""
    if args.executable:
        return [args.executable]
    return os.listdir(args.build_folder)

def generate_binary_flags(binary, args) -> str:
    """Generate the flags to run the binary"""
    flags_list = []

    if args.real_time:
        flags_list.append("-rt")
    else:
        flags_list.append("-no-rt")

    # Place in temporary storage. Outputs get copied into top level before killing the binary
    flags_list.append(f"-flash-mount={args.output}/flash_mount")
    flags_list.append(f"-flash-bin={binary}.bin")

    return " ".join(flags_list)

def run_simulation(start_barrier, stop_barrier, binary, args, output_folder):
    """Run the simulation for the given binary"""
    flags = generate_binary_flags(binary, args)
    start_barrier.wait()


    print(f"Running simulation for {binary}")

    # Stop once "RTOS Stopped!" is printed. Note that there is stuff before this too

    stop_barrier.wait()


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
    if not validate_arguments(args):
        return False

    binaries = get_binaries(args)
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
