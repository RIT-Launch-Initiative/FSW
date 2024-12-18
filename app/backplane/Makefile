# app/backplane/Makefile

# List of backplane modules
BACKPLANE_MODULES := power_module sensor_module radio_module

.PHONY: $(BACKPLANE_MODULES:%=-sim) $(BACKPLANE_MODULES:%=-flight) clean

# Pattern rules for backplane builds
%-sim: app/backplane/%/
	west build -b $(SIM_BOARD) $< -p auto $(OVERLAY_CONFIG) $(MULTIBUILD_ARGS)/$@

%-flight: app/backplane/%/
	west build -b $* $< -p auto $(MULTIBUILD_ARGS)/$@

# Clean target
clean:
	rm -rf build builds
