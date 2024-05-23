#ifndef  L_TFTP_H
#define L_TFTP_H

#include <zephyr/net/tftp.h>

/**
* Initialize the TFTP client
* @param client - Pointer to the client to initialize
*/
int l_tftp_init(struct tftpc *client);

/**
* Put a file onto a TFTP server in octet mode.
* @param client - Pointer to the client to use
* @param filename - Name of the file to put
*/
int l_tftp_put(const struct tftpc *const client, const char *const filename);

#endif //L_TFTP_H
