radio:
	west build -b radio_module app/backplane/radio_module -p auto -DDEBUG=y

radio-433:
	west build -b radio_module app/backplane/radio_module -p auto -DDEBUG=y -DEXTRA_CONF_FILE=433mhz.conf

radio-sim:
	west build -b native_sim app/backplane/radio_module -p auto -DDEBUG=y

radio-flight:
	west build -b radio_module app/backplane/radio_module -p auto

radio-flight-433:
	west build -b radio_module app/backplane/radio_module -p auto -DEXTRA_CONF_FILE=433mhz.conf
