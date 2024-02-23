all:
	west build

flash:
	west flash

debug:
	west debug

mon:
	sudo minicom -D /dev/ttyUSB0 -b 115200

# Modules
power:
	west build -b power_module@2 app_power_module -p auto -DOVERLAY_CONFIG=debug.conf

radio:
	west build -b radio_module app_radio_module -p auto -DOVERLAY_CONFIG=debug.conf

sensor:
	west build -b sensor_module app_sensor_module -p auto -DOVERLAY_CONFIG=debug.conf

# Extension boards
potato:
	west build -b potato app_potato -p auto -DOVERLAY_CONFIG=debug.conf
clean:
	rm -rf build

