power:
	west build -b power_module@2 app/backplane/power_module -p auto -DDEBUG=y

power-sim:
	west build -b native_sim app/backplane/power_module -p auto -DDEBUG=y

power-flight:
	west build -b power_module@2 app/backplane/power_module -p auto

blt:
	west build -b blt_plane app/backplane/power_module -p auto -DDEBUG=y

blt-flight:
	west build -b blt_plane app/backplane/power_module -p auto
