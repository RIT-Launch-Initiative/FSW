include app/backplane/sensor_module/Makefile
include app/backplane/power_module/Makefile

radio:
	west build -b radio_module app/backplane/radio_module -p auto

radio-rel:
	west build -b radio_module app/backplane/radio_module -p auto

radio-rcv:
	west build -b radio_module app/backplane/radio_module -p auto -DCONFIG_RADIO_MODULE_RECEIVER=y -DOVERLAY_CONFIG=receiver.conf

radio-rcv-rel:
	west build -b radio_module app/backplane/radio_module -p auto -DCONFIG_RADIO_MODULE_RECEIVER=y

