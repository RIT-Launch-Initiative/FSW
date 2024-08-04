# Launch Flight Software
Current source code for Launch's custom flight hardware utilizing Zephyr RTOS. This includes but is not limited to the Backplane (modules), GRIM and POTATO.  

### Compiling
There are 3 ways to compile a project:
- Run make <name of project> from the root of the repository. 
- Navigate to the directory of the application (`app/` folders) and run make there if there is a custom makefile.
- If no makefile contains that project, most applications can be built manually with
```sh
west build <PATH TO YOUR APP> -p auto -b <BOARD> [--shield=<YOUR SHIELD>]
```


### Running
Connect to board and run west flash. You must first build the project so west knows what project you want to flash.


