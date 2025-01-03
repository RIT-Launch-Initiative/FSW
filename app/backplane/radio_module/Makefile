ifeq ($(FSW_DISABLE_MULTIPLE_BUILDS),true)
MULTIBUILD_ARGS :=
echo "Multiple builds disabled"
else
MULTIBUILD_ARGS := --build-dir builds
endif

radio:
	west build -b radio_module app/backplane/radio_module -p auto -DDEBUG=y $(MULTIBUILD_ARGS)/$@

radio-sim:
	west build -b native_sim app/backplane/radio_module -p auto -DDEBUG=y

radio-flight:
	west build -b radio_module app/backplane/radio_module -p auto $(MULTIBUILD_ARGS)/$@

radio-rcv:
	west build -b radio_module app/backplane/radio_module -p auto -DCONFIG_RADIO_MODULE_RECEIVER=y -DDEBUG=y $(MULTIBUILD_ARGS)/$@
