#include <zephyr/fs/fs.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>

// read from file into this
// chosen so only the last line will pad with = when there is not enough data for a full line
// all previous lines will not need padding so the entire stream can be reassembled and decoded in one go
// chosen to be 60 to fit on 80 column terminal when encoded

#define READING_BUFFER_LEN 60
uint8_t reading_data[READING_BUFFER_LEN];

// dump base64 data here before printing
// overhead of (8 bits/byte -> 6 bits/byte) plus padding
#define DUMP_BUFFER_LEN ((READING_BUFFER_LEN * 4 / 3) + 2)
uint8_t dumping_data[DUMP_BUFFER_LEN];
static int dump_base64(const struct shell *shell, const char *fname) {
    struct fs_dirent dirent;
    int ret = fs_stat(fname, &dirent);
    if (ret < 0) {
        shell_print(shell, "Error getting file size: %d", ret);
        return ret;
    }
    shell_print(shell, "Size: %zu", dirent.size);

    struct fs_file_t file;
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

        shell_print(shell, "%.*s", DUMP_BUFFER_LEN, dumping_data);
    }
    fs_close(&file);
    return 0;
}
static int cmd_read_file(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (argc != 2) {
        shell_print(shell, "usage: fdd dump filename: dump specific file");
        return -1;
    }

    return dump_base64(shell, argv[1]);
}
static int cmd_tree(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Print tree");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(fdd_subcmds, SHELL_CMD(tree, NULL, "Show Tree of filesystem", cmd_tree),
                               SHELL_CMD(read, NULL, "Read file.", cmd_read_file), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(fdd, &fdd_subcmds, "Flight data dumper commands", NULL);
