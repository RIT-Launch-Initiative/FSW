#include "config.h"

#include <zephyr/fs/fs.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>

#ifdef CONFIG_SHELL

// read from file into this
#define B64_PER_LINE       (12 * 6)
#define READING_BUFFER_LEN B64_PER_LINE
uint8_t reading_data[READING_BUFFER_LEN];

// dump base64 data here before printing
#define DUMP_BUFFER_LEN (B64_PER_LINE * 8)
uint8_t dumping_data[DUMP_BUFFER_LEN + 1];

static int dump_base64(const struct shell *shell, const char *fname) {
    struct fs_file_t file;
    int ret;
    fs_file_t_init(&file);

    ret = fs_open(&file, fname, FS_O_READ);
    if (ret < 0) {
        shell_print(shell, "Failed to open %s: %d", fname, ret);
        return ret;
    }
    int read = READING_BUFFER_LEN;

    while (read == READING_BUFFER_LEN) {
        read = fs_read(&file, reading_data, READING_BUFFER_LEN);
        if (read < 0) {
            shell_print(shell, "Failed file read of %s", fname);
            return -1;
        }
        int num_written = 0;
        ret = base64_encode(dumping_data, DUMP_BUFFER_LEN, &num_written, reading_data, read);
        if (ret < 0) {
            shell_print(shell, "encoding error %d, num_written: %d, read: %d", ret, num_written, read);
            return ret;
        }
        dumping_data[DUMP_BUFFER_LEN] = 0; // null terminator
        shell_print(shell, "%s", dumping_data);
    }
    fs_close(&file);
    return 0;
}
#define SEPARATOR "********\n"
static int cmd_dump_file(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (argc < 1) {
        shell_print(shell, "usage: grim dump filename: dump specific file");
        shell_print(shell, "usage: grim dump: dump all files");
        return -1;
    }
    // Dump All
    if (argc == 1) {
        dump_base64(shell, SLOW_FILENAME);
        shell_print(shell, SEPARATOR);
        dump_base64(shell, FAST_FILENAME);
        shell_print(shell, SEPARATOR);
        dump_base64(shell, ADC_FILENAME);
        shell_print(shell, SEPARATOR);
        dump_base64(shell, PRELAUNCH_ACCEL_FILENAME);
        shell_print(shell, SEPARATOR);
        dump_base64(shell, PRELAUNCH_ALT_FILENAME);
    }
    // Dump Specific
    if (argc > 1) {
        dump_base64(shell, argv[1]);
        return 0;
    }
    return 0;
}

extern bool flight_cancelled;

static int cmd_nogo(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    flight_cancelled = true;

    return 0;
}

extern void describe_flight();

static int cmd_status(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    describe_flight();

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(grim_subcmds, SHELL_CMD(nogo, NULL, "Cancel launch detection", cmd_nogo),
                               SHELL_CMD(dump, NULL, "Dump  file.", cmd_dump_file),
                               SHELL_CMD(status, NULL, "Print flight information.", cmd_status), SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "demo" */
SHELL_CMD_REGISTER(grim, &grim_subcmds, "Grim Control Commands", NULL);

#endif