sensor:
	west build -b sensor_module app/backplane/sensor_module -p auto -DDEBUG=y

sensor-sim:
	west build -b native_sim app/backplane/sensor_module -p auto -DDEBUG=y

sensor-sim-no-primary-barom:
	west build -b native_sim app/backplane/sensor_module -p auto -DDEBUG=y --extra-dtc-overlay boards/broken_sensor_overlays/primary_barom_broken.overlay
sensor-sim-no-secondary-barom:
	west build -b native_sim app/backplane/sensor_module -p auto -DDEBUG=y --extra-dtc-overlay boards/broken_sensor_overlays/secondary_barom_broken.overlay

sensor-sim-no-primary-acc:
	west build -b native_sim app/backplane/sensor_module -p auto -DDEBUG=y --extra-dtc-overlay boards/broken_sensor_overlays/primary_imu_broken.overlay
sensor-sim-no-secondary-acc:
	west build -b native_sim app/backplane/sensor_module -p auto -DDEBUG=y --extra-dtc-overlay boards/broken_sensor_overlays/secondary_imu_broken.overlay

sensor-hw-sim:
	west build -b sensor_module app/backplane/sensor_module -p auto -DDEBUG=y --extra-dtc-overlay boards/sensor_hw_sim.overlay --extra-conf boards/sensor_hw_sim.conf

sensor-flight:
	west build -b sensor_module app/backplane/sensor_module -p auto
