# Top-Level Makefile

all:
	west build

flash:
	west flash

ci:
 	if [ "Linux" = "Windows" ]; then
    	EXTRA_TWISTER_FLAGS="--short-build-path -O/tmp/twister-out"
	fi
  	function add_arglist () {
    	echo "$2" | while read -r line; do
	      [ -z "$line" ] && continue
    	  echo -n " $1 $line "
		  done;
	}       
  
  west twister $(add_arglist -T "app/backplane/power_module
  app/backplane/radio_module
  app/backplane/sensor_module
  ") $(add_arglist -p "nucleo_f446re
  radio_module
  sensor_module
  power_module
  ") $(if [ "Linux" = "Linux" ]; then add_arglist -p "native_sim
  "; fi) -v --inline-logs --integration $EXTRA_TWISTER_FLAGS

clean:
	rm -rf build

include app/backplane/Makefile
include app/payload/Makefile
include app/other/Makefile
