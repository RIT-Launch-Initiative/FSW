BACKPLANE_MODULES := 
	power_module \
	sensor_module \
	radio_module

.PHONY: $(BACKPLANE_MODULES:%=-sim) $(BACKPLANE_MODULES:%=-flight) $(BACKPLANE_MODULES:%=-default) clean

%-sim: app/backplane/%/
	west build -b $(SIM_BOARD) $< -p auto $(OVERLAY_CONFIG) $(MULTIBUILD_ARGS)/$@

%-default: app/backplane/%/
	west build -b $* $< -p auto -DDEBUG=y $(OVERLAY_CONFIG) $(MULTIBUILD_ARGS)/$@

%-flight: app/backplane/%/
	west build -b $* $< -p auto $(MULTIBUILD_ARGS)/$@

clean:
	rm -rf build builds
