ifeq ($(FSW_DISABLE_MULTIPLE_BUILDS),true)
MULTIBUILD_ARGS :=
else
MULTIBUILD_ARGS := --build-dir builds
endif

SIM_BOARD := native_sim
F446_BOARD := nucleo_f446re
OVERLAY_CONFIG := -DOVERLAY_CONFIG=debug.conf

APPS := datalogger \
		filesystem \
		littlefs \
		message_passing \
		openrocket_sensors \
        soft_timer \
		tenants_and_tasks \
		udp_broadcast \

%-sim: app/samples/%/
	west build -b $(SIM_BOARD) $< -p auto $(OVERLAY_CONFIG) $(MULTIBUILD_ARGS)/$@

%-f446: app/samples/%/
	west build -b $(F446_BOARD) $< -p auto $(MULTIBUILD_ARGS)/$@

.PHONY: all $(APPS:%=-sim) $(APPS:%=-f446)
