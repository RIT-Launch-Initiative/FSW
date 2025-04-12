# Top-Level Makefile

all:
	west build

flash:
	west flash

clean:
	rm -rf build

include app/backplane/Makefile
include app/ground/Makefile
include app/payload/Makefile
include app/other/Makefile
