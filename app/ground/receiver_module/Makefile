receiver:
	west build -b radio_module app/ground/receiver_module -p auto -DDEBUG=y

receiver-433:
	west build -b radio_module app/ground/receiver_module -p auto -DDEBUG=y -DEXTRA_CONF_FILE=433mhz.conf

receiver-flight:
	west build -b radio_module app/ground/receiver_module -p auto

receiver-flight-433:
	west build -b radio_module app/ground/receiver_module -p auto -DEXTRA_CONF_FILE=433mhz.conf
