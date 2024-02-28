#include "orchestrator.h"
#include "config.h"
#include "sensors.h"
#include <stdint.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(orchestrator);

K_EVENT_DEFINE(launch_detected);
K_EVENT_DEFINE(noseover_detected);
K_EVENT_DEFINE(main_detected);
K_EVENT_DEFINE(flight_over);

const uint64_t not_yet_launched = ~0;
const uint32_t not_yet_launched_32 = ~0;
/// microseconds after boot that launch happened
/// updated by orchestation thread when it detect launch

uint64_t launch_time_from_boot_us = not_yet_launched;
uint64_t micros_since_boot() {
  uint64_t cycles = k_uptime_get();
  uint64_t us_since_boot = k_cyc_to_us_near64(cycles);
  return 0;
}

uint32_t useconds_since_launch() {
  if (launch_time_from_boot_us == not_yet_launched) {
    return not_yet_launched_32;
  }
  uint64_t cycles = k_uptime_ticks();
  uint64_t us_since_boot = k_cyc_to_us_near64(cycles);
  return (uint32_t)(us_since_boot - launch_time_from_boot_us);
}
enum Phase {
  Phase_LaunchDetecting,
  Phase_FlightCancelled,
  Phase_Boost,
  Phase_ReefEvents,
  Phase_UnderMain,
  Phase_Ground
};

volatile static bool override_boost_detect = false;
volatile static bool flight_cancelled = false;
volatile static enum Phase flight_phase = Phase_LaunchDetecting;

// pre-detect buffer
// struct ring_buf something somehting

int orchestrate() {
  // too read into
  struct fast_data dat;
  // On Pad
  flight_phase = Phase_LaunchDetecting;
  LOG_INF("Detecting boost");
  while (!override_boost_detect) {
    if (flight_cancelled) {
      flight_phase = Phase_FlightCancelled;
      return 0;
    }

    int ret = read_fast(&dat);
    // fill in timestamp (not since launch, just since boot + wrapping)

    // ring_buf.push(dat)
    // if is_launched(){break;}

    k_msleep(LAUNCH_DETECT_PHASE_IMU_ALT_SAMPLE_PERIOD_MS);
  }

  // Boost Detected
  flight_phase = Phase_Boost;
  k_event_set(&launch_detected, EVENT_HAPPENED);
  LOG_INF("Detecting Noseover");
  while (1) {

    int ret = read_fast(&dat);
    // fill in timestamp

    if (k_msgq_put(&fast_data_queue, &dat, K_NO_WAIT) != 0) {
      LOG_WRN("fast data queue full");
    }

    k_msleep(BOOST_PHASE_IMU_ALT_SAMPLE_PERIOD_MS);
    break;
  }

  // Nose Over Detected
  flight_phase = Phase_ReefEvents;
  k_event_set(&noseover_detected, EVENT_HAPPENED);
  LOG_INF("Detecting Under Main");

  while (1) {
    int ret = read_fast(&dat);
    // fill in timestamp

    if (k_msgq_put(&fast_data_queue, &dat, K_NO_WAIT) != 0) {
      LOG_WRN("fast data queue full");
    }

    k_msleep(REEF_PHASE_IMU_ALT_SAMPLE_PERIOD_MS);
    break;
  }

  // Main Detected
  flight_phase = Phase_UnderMain;
  k_event_set(&main_detected, EVENT_HAPPENED);

  LOG_INF("Detecting Ground");
  while (1) {
    if (k_msgq_put(&fast_data_queue, &dat, K_NO_WAIT) != 0) {
      LOG_WRN("fast data queue full");
    }

    k_msleep(MAIN_PHASE_IMU_ALT_SAMPLE_PERIOD_MS);
    break;
  }

  // Wacked the ground
  flight_phase = Phase_Ground;
  k_event_set(&flight_over, EVENT_HAPPENED);

  return 0;
}

#ifdef CONFIG_ORCHESTRATOR_SHELL

/**
 * Shell Commands for controlling orchestration
 * ie. dont launch, dump data, fake boost detect
 */
static int cmd_nogo(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  if (flight_phase != Phase_LaunchDetecting) {
    shell_print(shell,
                "The payload already launched and/or landed. Can't nogo\n");
  } else {
    shell_print(shell, "Will not fly. Power cycle to fly again");
    flight_cancelled = true;
  }
  return 0;
}

static int cmd_phase(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  switch (flight_phase) {
  case Phase_FlightCancelled:
    shell_print(shell, "Flight Cancelled");
    break;

  case Phase_LaunchDetecting:
    shell_print(shell, "Detecting Launch");
    break;

  case Phase_ReefEvents:
    shell_print(shell, "Watching for reef events");
    break;

  case Phase_UnderMain:
    shell_print(shell, "Under Main");
    break;

  case Phase_Ground:
    shell_print(shell, "Hit the ground");
    break;

  default:
    shell_print(shell, "UNKNOWN FLIGHT PHASE (you should be paniccing)");
    break;
  }

  return 0;
}

static int cmd_useconds(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  uint64_t cycles = k_uptime_ticks();
  uint64_t us_since_boot = k_cyc_to_us_near64(cycles);

  shell_print(shell, "%llu microseconds since boot", cycles);

  return 0;
}

static int cmd_override_boost_detect(const struct shell *shell, size_t argc,
                                     char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_print(shell, "overriding boost");
  override_boost_detect = true;

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    orch_subcmds, SHELL_CMD(nogo, NULL, "Cancel launch detection", cmd_nogo),
    SHELL_CMD(phase, NULL, "Show current phase of flight.", cmd_phase),
    SHELL_CMD(uptime, NULL, "Show uptime in useconds.", cmd_useconds),
    SHELL_CMD(override_boost, NULL, "Pretend boost happened",
              cmd_override_boost_detect),
    SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "demo" */
SHELL_CMD_REGISTER(orchestrate, &orch_subcmds, "Orchestration Commands", NULL);

#endif