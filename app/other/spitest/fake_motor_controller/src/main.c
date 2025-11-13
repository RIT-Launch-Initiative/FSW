/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
// #include <zephyr/kernel.h>

const struct device *spi_periph_dev = DEVICE_DT_GET(DT_NODELABEL(arduino_spi));

static struct k_poll_signal spi_periph_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_periph_done_sig);

static const struct spi_config spi_periph_cfg = {
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_SLAVE,
    .frequency = 4000000,
    .slave = 0,
};


static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_NODELABEL(cs_spi), gpios);
static struct gpio_callback button_cb_data;
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}


int main(void) {
    if (!device_is_ready(spi_periph_dev)) {
        printk("SPI periph device not ready!\n");
    }

	int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);


    k_poll_signal_init(&spi_periph_done_sig);

    struct k_poll_event events[1] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &spi_periph_done_sig),
    };

    uint8_t in_buf[12];
    struct spi_buf rx_buf = {
        .buf = in_buf,
        .len = sizeof(in_buf),
    };
    struct spi_buf_set rx_bufs = {.buffers = &rx_buf, .count = 1};
	printk("Waiting for gpio\n");
    while (1) {
        k_poll(events, 1, K_FOREVER);
        printk("CS Line Low");
        int ret = spi_read(spi_periph_dev, &spi_periph_cfg, &rx_bufs);
        if (ret < 0) {
            printk("Error reading spi: %d\n", ret);
        } else {
			printk("Read %d bytes from spi: ", ret);
			for (int i = 0; i < ret; i++){
				printk("%02x ", in_buf[i]);
			}
			printk("\n");
		}
        // spi_read_dt()
        k_msleep(1000);
    }
    return 0;
}
