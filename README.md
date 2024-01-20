# Launch Flight Software
Current source code for Launch's custom flight hardware utilizing Zephyr RTOS. This includes but is not limited to the Backplane (modules), GRIM and POTATO.  

### Compiling
To compile, you can look at the commands in the Makefile. Type make and the board name. If this is a backplane module, don't include the word module (i.e `make sensor`)
You can just run `make` with no arguments if you plan on compiling the same board again. If you compile for another baord, 
a pristine build should occur automatically.

### Running
Connect to board and run west flash.


