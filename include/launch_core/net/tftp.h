#ifndef  L_TFTP_H
#define L_TFTP_H

#include <zephyr/net/tftp.h>

#define L_TFTP_PORT "69"

/**
* Initialize the TFTP client
* @param client - Pointer to the client to initialize
* @param server_host_name - TFTP server to connect to
*/
int l_tftp_init(struct tftpc *client, const char *server_host_name);

/**
* Put a file onto a TFTP server in octet mode.
* @param client - Pointer to the client to use
* @param filename - Name of the file to put
*/
int l_tftp_put(struct tftpc *const client, const char *const filename);

#endif //L_TFTP_H
