PROJECT_ROOT := $(realpath ../../)

potato:
	west build -b potato $(PROJECT_ROOT)/app/other/app_potato -p auto -DOVERLAY_CONFIG=debug.conf -DCONFIG_COMPILER_SAVE_TEMPS=y

potato-rel:
	west build -b potato $(PROJECT_ROOT)/app/other/app_potato -p auto
