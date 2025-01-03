ifeq ($(FSW_ENABLE_MULTIPLE_BUILDS),true)
	MULTIBUILD_ARGS := --build-dir builds/$@
else
	MULTIBUILD_ARGS :=
endif

sensor:
	west build -b sensor_module app/backplane/sensor_module -p auto -DDEBUG=y $(MULTIBUILD_ARGS)

sensor-sim:
	west build -b native_sim app/backplane/sensor_module -p auto -DDEBUG=y $(MULTIBUILD_ARGS)

sensor-flight:
	west build -b sensor_module app/backplane/sensor_module -p auto $(MULTIBUILD_ARGS)
