power:
	west build -b power_module@2 app/backplane/power_module -p auto -DOVERLAY_CONFIG=debug.conf

power-rel:
	west build -b power_module@2 app/backplane/power_module -p auto

radio:
	west build -b radio_module app/backplane/radio_module -p auto -DOVERLAY_CONFIG=debug.conf

radio-rel:
	west build -b radio_module app/backplane/radio_module -p auto

radio-rcv:
	west build -b radio_module app/backplane/radio_module -p auto -DOVERLAY_CONFIG=debug.conf -DCONFIG_RADIO_MODULE_RECEIVER=y -DOVERLAY_CONFIG=receiver.conf

radio-rcv-rel:
	west build -b radio_module app/backplane/radio_module -p auto -DCONFIG_RADIO_MODULE_RECEIVER=y

sensor:
	west build -b sensor_module app/backplane/sensor_module -p auto -DOVERLAY_CONFIG=debug.conf

sensor-rel:
	west build -b sensor_module app/backplane/sensor_module -p auto
