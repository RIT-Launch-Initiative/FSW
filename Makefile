all:
	west build

flash:
	west flash

# Modules
power:
	west build -b power_module@2 app_power_module -p auto -DOVERLAY_CONFIG=debug.conf

radio:
	west build -b radio_module app_radio_module -p auto -DOVERLAY_CONFIG=debug.conf

sensor:
	west build -b sensor_module@2 app_sensor_module -p auto -DOVERLAY_CONFIG=debug.conf

# Extension boards
potato:
	west build -b potato app_potato -p auto -DOVERLAY_CONFIG=debug.conf

reefer:
	west build -b grim_reefer app_grim_reefer -p auto -DOVERLAY_CONFIG=debug.conf


# Modules (Release)
power-rel:
	west build -b power_module@2 app_power_module -p auto

radio-rel:
	west build -b radio_module app_radio_module -p auto

sensor-rel:
	west build -b sensor_module@2 app_sensor_module -p auto

# Extension boards
potato-rel:
	west build -b potato app_potato -p auto

reefer-rel:
	west build -b grim_reefer app_grim_reefer -p auto


clean:
	rm -rf build

