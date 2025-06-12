#include "zephyr/drivers/flash.h"

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/net/socket.h>
#include <stdio.h>
#include <string.h>

#define FLASH_AREA_ID 0 // Set this to the correct flash area ID
#define TCP_PORT 9000
#define CHUNK_SIZE 1024
#define FLASH_CHUNK_SIZE 1024

ssize_t read_entire_flash(uint8_t **buffer) {
    const flash_area *fa;
    int rc = flash_area_open(FLASH_AREA_ID, &fa);
    if (rc != 0) {
        return -1;
    }
    const device *dev = flash_area_get_device(fa);
    uint64_t size;
    int ret = flash_get_size(dev, &size);
    if (ret != 0) {
        flash_area_close(fa);
        return -1;
    }

    *buffer = static_cast<uint8_t*>(k_malloc(size));
    if (!*buffer) {
        flash_area_close(fa);
        return -2;
    }
    rc = flash_area_read(fa, 0, *buffer, size);
    flash_area_close(fa);
    if (rc != 0) {
        k_free(*buffer);
        return -3;
    }
    return size;
}

int send_flash_over_tcp(const uint8_t *buffer, size_t size) {
    int server_fd = 0;
    int client_fd = 0;
    sockaddr_in addr = {0};
    sockaddr_in client_addr = {0};
    socklen_t addrlen = sizeof(client_addr);
    int ret = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) return -1;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd);
        return -2;
    }
    if (listen(server_fd, 1) < 0) {
        close(server_fd);
        return -3;
    }
    client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);
    if (client_fd < 0) {
        close(server_fd);
        return -4;
    }
    size_t sent = 0;
    while (sent < size) {
        ssize_t n = send(client_fd, buffer + sent, size - sent > CHUNK_SIZE ? CHUNK_SIZE : size - sent, 0);
        if (n <= 0) {
            ret = -5;
            break;
        }
        sent += n;
    }
    close(client_fd);
    close(server_fd);
    return ret;
}

int write_flash_to_file(const char *filename, const uint8_t *buffer, size_t size) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    size_t written = fwrite(buffer, 1, size, f);
    fclose(f);
    return (written == size) ? 0 : -2;
}

int send_flash_over_tcp_chunked(void) {
    const struct flash_area *fa;
    int rc = flash_area_open(FLASH_AREA_ID, &fa);
    if (rc != 0) {
        printk("Failed to open flash area: %d\n", rc);
        return -1;
    }

    // Fixed size for W25Q512 in bytes
    #define FLASH_SIZE 256
    #define FLASH_CHUNK_SIZE 64  // Smaller chunk size

    uint8_t *buffer = (uint8_t *)k_malloc(FLASH_CHUNK_SIZE);
    if (!buffer) {
        flash_area_close(fa);
        printk("Failed to allocate buffer\n");
        return -2;
    }

    int server_fd = 0;
    int client_fd = 0;
    sockaddr_in addr = {0};
    sockaddr_in client_addr = {0};
    socklen_t addrlen = sizeof(client_addr);
    int ret = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        k_free(buffer);
        printk("Failed to create socket: %d %d\n", server_fd, errno);
        return -3;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, "10.0.0.2", &addr.sin_addr);
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd);
        k_free(buffer);
        printk("Failed to bind socket: %d\n", errno);
        return -4;
    }

    printk("Listening on port %d...\n", TCP_PORT);
    if (listen(server_fd, 1) < 0) {
        close(server_fd);
        k_free(buffer);
        printk("Failed to listen\n");
        return -5;
    }

    printk("Waiting for client connection...\n");
    client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);
    if (client_fd < 0) {
        close(server_fd);
        k_free(buffer);
        printk("Failed to accept connection\n");
        return -6;
    }
    printk("Client connected, starting transfer of %d bytes\n", FLASH_SIZE);

    // Send the total size first as a 4-byte value
    uint32_t total_size = FLASH_SIZE;
    send(client_fd, &total_size, sizeof(total_size), 0);

    size_t offset = 0;
    size_t last_percent = 0;
    while (offset < FLASH_SIZE) {
        size_t chunk = (FLASH_SIZE - offset > FLASH_CHUNK_SIZE) ?
                       FLASH_CHUNK_SIZE : (FLASH_SIZE - offset);
        rc = flash_area_read(fa, offset, buffer, chunk);
        if (rc != 0) {
            printk("Flash read error at offset %zu: %d\n", offset, rc);
            ret = -7;
            break;
        }

        // send chunk to client
        ssize_t sent = send(client_fd, buffer, chunk, 0);
        if (sent != chunk) {
            printk("TCP send error: sent %zd of %zu\n", sent, chunk);
            ret = -8;
            break;
        }
        offset += chunk;

        // Print progress every 5%
        size_t percent = (offset * 100) / FLASH_SIZE;
        if (percent >= last_percent + 5 || offset == FLASH_SIZE) {
            printk("Transfer progress: %zu%%\n", percent);
            last_percent = percent;
        }
    }

    printk("Transfer complete: %zu bytes\n", offset);
    k_free(buffer);
    close(client_fd);
    close(server_fd);
    flash_area_close(fa);
    return ret;
}

int main() {
    printk("Starting SPI Flash TCP dump utility\n");

    if (send_flash_over_tcp_chunked() != 0) {
        printk("Failed to send flash over TCP\n");
        return 1;
    }

    printk("Flash dump complete\n");
    return 0;
}
