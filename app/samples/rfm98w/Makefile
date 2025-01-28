all: 
	west build
nucleo_f4:
	west build -b nucleo_f446re --shield launch_mikroe . -p auto -- -DSHIELD="launch_mikroe"

flash:
	west flash

clean:
	rm -rf build

udev:
	sudo service udev restart
	sudo udevadm control --reload
