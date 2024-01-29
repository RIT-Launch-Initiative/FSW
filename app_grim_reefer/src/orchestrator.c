#include <stdint.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys_clock.h>
const uint64_t not_yet_launched = ~0;
const uint32_t not_yet_launched_32 = ~0;
/// microseconds after boot that launch happened
/// updated by orchestation thread when it detect launch

uint64_t launch_time_from_boot_us = not_yet_launched;
uint64_t micros_since_boot() {
  uint64_t cycles = k_uptime_get();
  uint64_t us_since_boot = k_cyc_to_us_near64(cycles);
}

uint32_t useconds_since_launch() {
  if (launch_time_from_boot_us == not_yet_launched) {
    return not_yet_launched_32;
  }
  uint64_t cycles = k_uptime_ticks();
  uint64_t us_since_boot = k_cyc_to_us_near64(cycles);
  return (uint32_t)(us_since_boot - launch_time_from_boot_us);
}

/**
 * Shell Commands for controlling orchestration
 * ie. dont launch, dump data instead
 */
static int cmd_useconds(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  uint64_t cycles = k_uptime_ticks();
  uint64_t us_since_boot = k_cyc_to_us_near64(cycles);

  shell_print(shell, "%llu microseconds since boot\n", cycles);

  return 0;
}
/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_demo, SHELL_CMD(uptime, NULL, "Show uptime in useconds.", cmd_useconds),
    SHELL_SUBCMD_SET_END);
/* Creating root (level 0) command "demo" */
SHELL_CMD_REGISTER(orchestrator, &sub_demo, "Demo commands", NULL);
