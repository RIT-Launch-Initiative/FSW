# TestFlight: RIT Launch Initiative's SIL Tool for FSW

TestFlight is a **Software-in-the-Loop (SIL) simulation tool** for quickly testing and validating
FSW native_sim binaries and producing its outputs (filesystem, Ethernet packets, etc.) 

## Features
- **Flexible Binary Management**: Run a single binary or multiple binaries from a specified folder.
- **Real-Time Simulation**: Option to execute binaries in real-time or without real-time constraints.
- **Structured Logging**: Captures and categorizes warnings and errors.
- **Multithreaded Execution**: Runs simulations concurrently.
- **Organized Output**: Automatically organizes output files and logs into timestamped folders.

## Usage

### Command-Line Arguments
TestFlight accepts the following command-line arguments:

| Argument               | Description                                              | Required/Default       |
|------------------------|----------------------------------------------------------|------------------------|
| `-e, --executable`     | Path to a single executable to run in the simulation.    | Required (if `-b` not provided) |
| `-b, --build_folder`   | Path to a folder containing multiple executables.        | Required (if `-e` not provided) |
| `-o, --output`         | Path to the folder where outputs will be saved.          | Default: `tf_out`     |
| `-rt, --real-time`     | Run the simulation in real-time mode.                    | Default: `False`      |

### Example Usage

#### Run a Single Binary
To run a single binary:
```bash
python3 testflight.py -e /path/to/binary -o /path/to/output
```

#### Run Multiple Binaries from a Folder
To execute all binaries in a folder:
```bash
python3 testflight.py -b /path/to/build_folder -o /path/to/output
```

#### Enable Real-Time Mode
To enable real-time mode during execution:
```bash
python3 testflight.py -e /path/to/binary -rt
```

## Logs and Outputs

1. **Output Folder**:
   - All logs and outputs are stored in a timestamped subdirectory under the specified output folder (default: `tf_out`).
   - Example structure:
     ```
     tf_out/
       2023-01-01_12-00-00/
         binary_1/
            binary_1.log
            flash_mount/
            fs/
         binary_2/
            binary_2.log
            flash_mount/
            fs/
         simulation.log
     ```

2. **Simulation Logs**:
   - TestFlight logs are written to `simulation.log` (both console and file).
   - Individual binary logs are stored in the respective binary folders.

3. **Warnings and Errors**:
   - Warnings and errors for each binary are stored and displayed after the simulation grouped by binary.

## How It Works

1. **Argument Validation**:
   - Ensures either `-e` or `-b` is provided and validates paths.

2. **Binary Detection**:
   - If `-e` is specified, runs the given executable.
   - If `-b` is specified, detects all executables in the folder.

3. **Simulation Setup**:
   - Creates an output folder.
   - Generates flags for each binary based on real-time settings and output paths.

4. **Concurrent Execution**:
   - Runs each native_sim binary in a separate thread.
   - Synchronizes start and stop times using thread barriers.

5. **Log Management**:
   - Captures standard output and error streams.
   - Suppresses known, ignorable warnings and errors.

6. **Results**:
   - Displays all captured warnings and errors categorized by binary.
