
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
        shell_print(shell, "Potoato cant print all speciy a pasth\n");
    }
    // Dump Specific
    if (argc > 1) {
        dump_base64(shell, argv[1]);
        return 0;
    }
    return 0;
}

extern bool boost_detected;

static int cmd_go(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    boost_detected = true;

    return 0;
}

// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE(pot_subcmds, 
    SHELL_CMD(go, NULL, "Override launch detection", cmd_go),
    SHELL_CMD(dump, NULL, "Dump  file.", cmd_dump_file), 
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(potato, &pot_subcmds, "Potato Control Commands", NULL);


#endif