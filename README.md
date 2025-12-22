# Launch Flight Software
Current source code for Launch's custom flight hardware utilizing Zephyr RTOS. This includes but is not limited to the Backplane (modules), GRIM and POTATO.  

## Installation

### All OSes
1. Find a proper subdirectory to clone this repository into. You will have other folders at the same level in this subdirectory with FSW too.
2. Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) for installing west.
3. Run ```west init -l FSW``` where FSW is referencing the cloned repository
4. Run ```west update```
5. You should now have 3 directories ```FSW```, ```zephyr``` and ```modules``` 
6. Continue to other sections based on your OS

### Linux 
7. Just reference the rest of the guide for installing the Zephyr SDK :)
8. Confirm you can compile software. See below sections of this README. Work with teammates and avionics lead if issues arise.

### Windows and MacOS
7. See the below section on Docker Environment
8. Confirm you can compile software. See below sections of this README. Work with teammates and avionics lead if issues arise.

## Docker Environment (Non-Linux Users)
NOTE: Non-Linux users will still need to setup Zephyr for now including cloning the FSW repo, setting up west and getting the necessary Zephyr modules downloaded for now through west update. This should hopefully go away in the future. The reason is that the Docker setup mounts the FSW, zephyr and modules directories for now  

This project includes a pre-configured development container that sets up the necessary dependencies for running in an Ubuntu environment. Dev Containers were mainly designed for VSCode, but should work with IDEs like CLion. Docker Compose is also available to run outside of an IDE. Although you can build FSW for hardware, simulation is only supported in Zephyr due to OS limitations.

#### Quick Start
1. Open the FSW repository in VS Code
2. When prompted, click "Reopen in Container" or run the command "Dev Containers: Reopen in Container"
3. The container will automatically build and configure the environment for use.

## Manual Docker Setup
If you prefer to use Docker directly without VS Code:

```bash
# Start the development environment
make dev-start

# Enter the container shell
make docker-shell

# Stop the development environment
make dev-stop

# Rebuild and restart
make dev-restart
```

#### Container Features
- **Ubuntu 24.04** base image
- **Zephyr SDK 0.16.8** pre-installed
- **West build tool** configured
- **Python virtual environment** with Zephyr dependencies
- **Cross-compilation tools** for ARM targets
- **Simulation support** with 32-bit libraries
- **Git, CMake, Ninja** and other development tools

#### Environment Variables
The container automatically sets up:
- `ZEPHYR_BASE=/workspace/zephyr`
- `ZEPHYR_TOOLCHAIN_VARIANT=zephyr`
- `ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.16.8`

#### Workspace Structure
The container mounts three directories:
- `/workspace/FSW` - This repository
- `/workspace/zephyr` - Zephyr RTOS source
- `/workspace/modules` - Additional Zephyr modules

#### Troubleshooting
- Container logs can be viewed with: `make docker-logs`

## Creating New Projects

You can create a new Zephyr project from a template using the `create-project` west command.

### Usage
```bash
west create-project <project-name> <location>
```

### Arguments
- `<project-name>`: Name of the new project (e.g., `my_project`, `sensor_module`)
- `<location>`: Location where the project should be created (e.g., `app/samples`, `app/backplane`)

### Examples
```bash
# Create a new sample project
west create-project my_sample app/samples

# Create a new backplane module
west create-project new_module app/backplane

# Create a project with hyphens in the name
west create-project my-cool-project app/samples
```

### What it does
The command will:
1. Copy the template project from `app/samples/.template-project`
2. Customize the project files with your specified name
3. Create the project in the specified location

The generated project includes:
- `CMakeLists.txt` - Build configuration
- `prj.conf` - Project configuration
- `Kconfig` - Kconfig settings
- `sample.yaml` - Sample metadata
- `src/main.cpp` - Main source file

After creating the project, you can build it with:
```bash
west build -b <board> <location>/<project-name>
```

## Ways to Compile FSW Applications
There are 3 different ways to compile a project:
- Run make <name of project> from the root of the repository. 
- Navigating to the directory of the application (`app/` folders) and run make there if there is a custom makefile.
- If no makefile contains that project, most applications can be built manually with
```west build <PATH TO YOUR APP> -p auto -b <BOARD> [--shield=<YOUR SHIELD>]```

### Examples
- ```make sensor```
- ```make sensor-sim```
- ```west build -b sensor_module app/backplane/sensor_module```
- ```west build -b native_sim app/backplane/sensor_module```

## Running on Simulation
*Note that some projects will not be able to fully support simulations. For example, LoRa communication will not be able to be simulated or monitoring different sensors.
For sensors, we swap the sensor driver with a fake sensor driver that has a lookup table built at compile time which is based on generated CSV files.*

*Reference https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html for more information on native_sim*

### Compiling
*Currently only power and sensor module are the only flight hardware projects supporting simulation*
- ```make sensor``` would become ```make sensor-sim```
- ```make power``` would become ```make power-sim```

The board should be native_sim in all cases for running simulations. Once the project is built, you can execute the simulation in two ways
- ```west build -t run```
- ```{BUILD_DIRECTORY}/zephyr/zephyr.exe```

Normally appending a -sim to the project name when running make should work.

### Ethernet
If you are running an application that uses Ethernet, you can Wireshark your packets as if you were connected to actual flight hardware.
However, the zeth network interface may not be showing in Wireshark and/or you notice that the simulation logs state 
```[00:00:00.000,000] <err> eth_posix: Cannot create zeth (-1/Operation not permitted)```

To set up the network interface, you must clone https://github.com/zephyrproject-rtos/net-tools and run ```net-setup.sh start``` to turn on the network interface. You can also run ```net-setup.sh stop``` to turn it off. It is recommended to alias ```{DIRECTORY_PATHS_TO_net-tools}/net-setup.sh``` so you can run this command anywhere. Once its started, you will notice znet as an interface in Wireshark which you can sniff for packets and running the simulation will no longer log the error.

*Your firewall may block this in rare cases, so if you're still running into issues, double check your firewall*

### Data Persistence
Some applications also involve logging data to flash. This is also simulated. The entire fileysystem will be written to flash.bin and if the project is properly configured, there should
also be a folder called flash where you can access data natively. Note that the folder (not binary), will have its contents wiped once you exit the simulation, so keep the simulation running while you are trying to access its data.

- If you ran the simulation through the executable, flash will be output in your current working directory.
- If you ran the simulation through west, flash will be output in your build directory.

## Running on Flight Hardware
Connect to board and run west flash. You must first build the project so west knows what project you want to flash.


