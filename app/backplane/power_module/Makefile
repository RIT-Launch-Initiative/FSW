power:
	west build -b power_module@2 app/backplane/power_module -p auto -DDEBUG=y $(MULTIBUILD_ARGS)/$@

power-sim:
	west build -b native_sim app/backplane/power_module -p auto -DDEBUG=y $(MULTIBUILD_ARGS)/$@

power-flight:
	west build -b power_module@2 app/backplane/power_module -p auto
