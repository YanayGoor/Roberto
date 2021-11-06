#include "drivers/enc28j60/internal.h"

#include <io/enc28j60.h>
#include <io/gpio.h>
#include <io/spi.h>
#include <malloc.h>
#include <memory.h>
#include <stdbool.h>

#define SIZEOF_ARR(x) (sizeof(x) / sizeof(*(x)))

#define LONG_DELAY 400000

#define GREEN_LED  12
#define ORANGE_LED 13
#define RED_LED	   14
#define BLUE_LED   15

#define MAX_FRAME_LEN 1518

const struct gpio_pin onboard_LEDs[] = {
	{.pin = GREEN_LED, .mode = GPIO_OUTPUT},
	{.pin = ORANGE_LED, .mode = GPIO_OUTPUT},
	{.pin = RED_LED, .mode = GPIO_OUTPUT},
	{.pin = BLUE_LED, .mode = GPIO_OUTPUT},
};

const struct spi_params enc28j60_spi_params = {.sclk_port = &gpio_pb,
											   .sclk_pin = 3,
											   .miso_port = &gpio_pb,
											   .miso_pin = 4,
											   .mosi_port = &gpio_pb,
											   .mosi_pin = 5,
											   .is_master = true,
											   .baud_rate = 7};

const struct spi_slave enc8j60_spi_slave = {
	.ss_port = &gpio_pb,
	.ss_pin = 6,
};

void delay(int weight) {
	for (int i = 0; i < weight; i++) {}
}

void flash(uint8_t on) {
	gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[on].pin);
	delay(LONG_DELAY);
	gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[on].pin);
	delay(LONG_DELAY);
}

void done() {
	while (1) {}
}

int main() {
	struct enc28j60_controller enc = {0};
	gpio_init(&gpio_pd, onboard_LEDs, SIZEOF_ARR(onboard_LEDs));

	spi_init(&spi_module_1, enc28j60_spi_params);

	// TODO: separate init to struct and hardware or automatically reset
	enc28j60_init(&enc, &spi_module_1, &enc8j60_spi_slave, true, MAX_FRAME_LEN,
				  1, 1);
	enc28j60_reset(&enc);
	delay(LONG_DELAY);
	enc28j60_init(&enc, &spi_module_1, &enc8j60_spi_slave, true, MAX_FRAME_LEN,
				  1, 1);

	uint16_t macon1 = enc28j60_read_ctrl_reg(&enc, ENC28J60_MACON1);
	uint16_t macon3 = enc28j60_read_ctrl_reg(&enc, ENC28J60_MACON3);
	uint16_t econ1 = enc28j60_read_ctrl_reg(&enc, ENC28J60_ECON1);
	uint16_t max_frame_len = enc28j60_read_ctrl_reg(&enc, ENC28J60_MAMXFL);
	uint16_t read_buffer_start = enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXST);
	uint16_t read_buffer_end = enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXND);
	uint16_t read_buffer_read_start =
		enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXRDPT);
	uint16_t macon4 = enc28j60_read_ctrl_reg(&enc, ENC28J60_MACON4);
	uint16_t mabb = enc28j60_read_ctrl_reg(&enc, ENC28J60_MABBIPG);
	uint16_t mai = enc28j60_read_ctrl_reg(&enc, ENC28J60_MAIPG);
	uint16_t filters = enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXFCON);
	uint16_t phid1 = enc28j60_read_phy_ctrl_reg(&enc, 0x02);
	uint16_t phcon1 = enc28j60_read_phy_ctrl_reg(&enc, 0x00);

	struct enc28j60_pkt_rx_hdr *hdr =
		malloc(sizeof(struct enc28j60_pkt_rx_hdr));
	uint8_t *buff = malloc(enc.max_frame_length);

	uint16_t received = 0;
	uint16_t read = 0;
	uint16_t err = 0;
	while (1) {
		err = enc28j60_read_ctrl_reg(&enc, ENC28J60_EIR) & 1;
		if (err) { gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[2].pin); }
		received = enc28j60_read_ctrl_reg(&enc, ENC28J60_EPKTCNT);
		if (!received) {
			gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[0].pin);
			continue;
		}
		gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[0].pin);
		gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[1].pin);
		int success =
			enc28j60_receive_packet(&enc, hdr, buff, enc.max_frame_length);
		if (success < 0) {
			gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[3].pin);
		} else {
			read++;
			gpio_write_partial(&gpio_pd, 0, 0xffff);

			if (!(enc28j60_read_ctrl_reg(&enc, ENC28J60_ECON1) & (1 << 3))) {
				uint8_t src_mac[6] = {1, 3, 3, 7, 9, 9};

				memcpy(buff, buff + 6, 6);
				memcpy(buff + 6, src_mac, 6);
				enc28j60_transmit_packet(&enc, 0, (uint8_t *)buff,
										 hdr->byte_count);
			}
		}
	}

	done();
}
