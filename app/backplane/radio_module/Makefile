radio:
	west build -b radio_module app/backplane/radio_module -p auto -DDEBUG=y $(MULTIBUILD_ARGS)/$@

radio-flight:
	west build -b radio_module app/backplane/radio_module -p auto $(MULTIBUILD_ARGS)/$@

radio-rcv:
	west build -b radio_module app/backplane/radio_module -p auto -DCONFIG_RADIO_MODULE_RECEIVER=y -DDEBUG=y $(MULTIBUILD_ARGS)/$@
