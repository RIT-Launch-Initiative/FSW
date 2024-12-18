# Top-Level Makefile

ifeq ($(FSW_DISABLE_MULTIPLE_BUILDS),true)
MULTIBUILD_ARGS :=
else
MULTIBUILD_ARGS := --build-dir builds
endif

all:
	west build

flash:
	west flash

clean:
	rm -rf build

include app/backplane/Makefile
include app/payload/Makefile
include app/other/Makefile