radio:
	west build -b receiver_module app/backplane/receiver_module -p auto -DDEBUG=y

radio-sim:
	west build -b native_sim app/backplane/receiver_module -p auto -DDEBUG=y

radio-flight:
	west build -b receiver_module app/backplane/receiver_module -p auto

radio-rcv:
	west build -b receiver_module app/backplane/receiver_module -p auto -DCONFIG_RADIO_MODULE_RECEIVER=y -DDEBUG=y
