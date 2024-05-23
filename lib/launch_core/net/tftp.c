#include <launch_core/net/tftp.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(l_tftp);

int l_tftp_init(struct tftpc *client, const char *server_host_name) {
    struct sockaddr remote_addr = {0};
    struct addrinfo *res = {0};
    struct addrinfo hints = {0};

    /* Setup TFTP server address */
    hints.ai_socktype = SOCK_DGRAM;

    int ret = getaddrinfo(server_host_name, L_TFTP_PORT, &hints, &res);
    if (ret != 0) {
        LOG_ERR("Unable to resolve address");
        return -ENOENT;
    }

    memcpy(&remote_addr, res->ai_addr, sizeof(remote_addr));
    freeaddrinfo(res);

    /* Save sockaddr into TFTP client handler */
    memcpy(&client->server, &remote_addr, sizeof(client->server));

    LOG_INF("Successfully initialized TFTP client");

    return 0;
}

int l_tftp_put(struct tftpc *const client, const char *const filename) {
    static const char* mode = "octet";
    static const uint8_t* test_data = "Hello, World!";

    int ret = tftp_put(client, filename, mode, test_data, 13);
    if (ret < 0) {
        LOG_ERR("Failed to put file through TFTP");
    }

    return ret;
}

