# Backplane Flight Computer
Source code for Launch's Backplane Flight Computer modules utilizing Zephyr RTOS

### Compiling
To compile, you can look at the commands in the Makefile. Type make and the board name without the module (i.e `make sensor`)
You can just run `make` with no arguments if you plan on compiling the same board again. If you compile for another baord, 
a pristine build should occur automatically.

### Running
Connect to board and run west flash.


