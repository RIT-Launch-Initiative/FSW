# Launch Flight Software
Current source code for Launch's custom flight hardware utilizing Zephyr RTOS. This includes but is not limited to the Backplane (modules), GRIM and POTATO.  

### Compiling
To compile, navigate to the directory of the application (`app/` folders) and look for a makefile.
If no makefile is found, most applications can be built with 
```sh
west build <PATH TO YOUR APP> -p auto -b <BOARD> [--shield=<YOUR SHIELD>]
```
examples:


### Running
Connect to board and run west flash.


