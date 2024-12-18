# Top-Level Makefile

# Disable multiple boards if FSW_DISABLE_MULTIPLE_BUILDS is set to true
ifeq ($(FSW_DISABLE_MULTIPLE_BUILDS),true)
MULTIBUILD_ARGS :=
else
MULTIBUILD_ARGS := --build-dir builds
endif

# Exports for child Makefiles
export MULTIBUILD_ARGS
export SIM_BOARD := native_sim
export F446_BOARD := nucleo_f446re
export OVERLAY_CONFIG := -DOVERLAY_CONFIG=debug.conf

# Subdirectories
SUBDIRS := \ 
	app/backplane \ 
	app/payload \ 
	app/other \
	app/samples

.PHONY: all clean flash

all:
	@echo "Specify a target or navigate to a subdirectory."

# Clean up build directories across all subdirectories
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

flash:
	west flash

$(SUBDIRS):
	$(MAKE) -C $@

clean-%:
	$(MAKE) -C $* clean
