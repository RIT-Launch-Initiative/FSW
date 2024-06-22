/****                   README                      ****/
/* Requires a TFTP server to be setup on host machine */
/* For Linux, you can use tftpd-hpa                   */
/* Can set wherever you want files to be stored       */
/* Default is /tftpboot or /var/lib/tftpboot          */
/* Due to Zephyr limits, files must be created first */
/* Make sure permissions are set up                  */
/* chmod +666 on created files                       */
/****                   END                      ****/

#ifndef L_TFTP_H
#define L_TFTP_H

#include <zephyr/net/tftp.h>

#define L_TFTP_PORT "69"
#define L_DEFAULT_SERVER_IP "10.0.0.0"
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
* @param buffer - Buffer of data to send over TFTP
* @param buffer_size - Size of the buffer
*/
int l_tftp_put(struct tftpc *const client, const char *const filename, const uint8_t *buff, const size_t buff_size);

/**
* Initialize and put a file onto a TFTP server in octet mode.
* @param client - IP address of the TFTP server
* @param filename - Name of the file to put
* @param buffer - Buffer of data to send over TFTP
* @param buffer_size - Size of the buffer
*/
int l_tftp_init_and_put(const char *ip, const char* fname, uint8_t *buff, size_t buff_size);


#endif //L_TFTP_H
