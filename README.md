# Launch Flight Software
Current source code for Launch's custom flight hardware utilizing Zephyr RTOS. This includes but is not limited to the Backplane (modules), GRIM and POTATO.  

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


