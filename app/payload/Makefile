PROJECT_ROOT := $(realpath ../../)

reefer:
	west build -b grim_reefer $(PROJECT_ROOT)/app/payload/app_grim_reefer -p auto -DOVERLAY_CONFIG=debug.conf

reefer-rel:
	west build -b grim_reefer $(PROJECT_ROOT)/app/payload/app_grim_reefer -p auto
