#include "drivers/enc28j60/internal.h"

#include <io/enc28j60.h>
#include <io/gpio.h>
#include <io/spi.h>
#include <malloc.h>
#include <memory.h>
#include <stdbool.h>

#define SIZEOF_ARR(x) (sizeof(x) / sizeof(*(x)))

#define SHORT_DELAY 800000
#define LONG_DELAY	1600000

#define FLASHES 3

#define GREEN_LED  12
#define ORANGE_LED 13
#define RED_LED	   14
#define BLUE_LED   15

#define GREEN_LED_MASK	(1 << GREEN_LED)
#define ORANGE_LED_MASK (1 << ORANGE_LED)
#define RED_LED_MASK	(1 << RED_LED)
#define BLUE_LED_MASK	(1 << BLUE_LED)
#define LEDS_MASK                                                              \
	(GREEN_LED_MASK | ORANGE_LED_MASK | RED_LED_MASK | BLUE_LED_MASK)

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

extern int samples[];
extern int samples_i;

void flash(uint8_t on) {
	gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[on].pin);
	delay(800000);
	gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[on].pin);
	delay(200000);
}

void flash3(uint8_t on0, uint8_t on1, uint8_t on2, uint8_t on3) {
	//	gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[on].pin);
	if (on0) { gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[0].pin); }
	if (on1) { gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[1].pin); }
	if (on2) { gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[2].pin); }
	if (on3) { gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[3].pin); }
	delay(1600000);
	//	gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[on].pin);
	if (on0) { gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[0].pin); }
	if (on1) { gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[1].pin); }
	if (on2) { gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[2].pin); }
	if (on3) { gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[3].pin); }
	delay(400000);
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
	delay(1000000);
	enc28j60_init(&enc, &spi_module_1, &enc8j60_spi_slave, true, MAX_FRAME_LEN,
				  1, 1);

	//	enc28j60_write_ctrl_reg(&enc, ENC28J60_ECON1, 4);
	//	enc28j60_write_ctrl_reg(&enc, ENC28J60_MACON1, 3);
	//	enc28j60_write_ctrl_reg(&enc, ENC28J60_MACON3, 5);

	uint16_t macon1 = enc28j60_read_ctrl_reg(&enc, ENC28J60_MACON1);
	uint16_t macon3 = enc28j60_read_ctrl_reg(&enc, ENC28J60_MACON3);
	uint16_t econ1 = enc28j60_read_ctrl_reg(&enc, ENC28J60_ECON1);
	//	uint16_t max_frame_len =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_MAMXFL);
	//	uint16_t read_buffer_start =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXST);
	//	uint16_t read_buffer_end =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXND);
	//	uint16_t read_buffer_read_start =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXRDPT);
	//	uint16_t macon4 =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_MACON4);
	//	uint16_t mabb =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_MABBIPG);
	//	uint16_t mai =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_MAIPG);
	//	uint16_t filters =
	//		enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXFCON);
	uint16_t phid1 = enc28j60_read_phy_ctrl_reg(&enc, 0x02);
	uint16_t phcon1 = enc28j60_read_phy_ctrl_reg(&enc, 0x00);

	uint16_t received = 0;
	struct enc28j60_pkt_rx_hdr *prev2_hdr =
		malloc(sizeof(struct enc28j60_pkt_rx_hdr));
	struct enc28j60_pkt_rx_hdr *prev_hdr =
		malloc(sizeof(struct enc28j60_pkt_rx_hdr));
	struct enc28j60_pkt_rx_hdr *hdr =
		malloc(sizeof(struct enc28j60_pkt_rx_hdr));
	uint8_t *buff = malloc(enc.max_frame_length);
	int read = 0;
	uint16_t prev_err = 0;
	uint16_t prev2_err = 0;
	uint16_t err = 0;
	uint16_t errs = 0;
	while (read < 300) {
		prev2_err = prev_err;
		prev_err = err;
		err = enc28j60_read_ctrl_reg(&enc, ENC28J60_EIR) & 1;
		uint16_t wanna_write_at =
			enc28j60_read_ctrl_reg(&enc, ENC28J60_ERXWRPT);
		received = enc28j60_read_ctrl_reg(&enc, ENC28J60_EPKTCNT);
		if (received) {
			int success =
				enc28j60_receive_packet(&enc, hdr, buff, enc.max_frame_length);
			if (success > 0) {
				memcpy(prev2_hdr, prev_hdr, sizeof(struct enc28j60_pkt_rx_hdr));
				memcpy(prev_hdr, hdr, sizeof(struct enc28j60_pkt_rx_hdr));
				read++;
				if (errs) { errs = 0; }
				flash(1);
			} else if (errs < 100) {
				errs++;
			} else {
				// cry
				int a = 5;
			}
		}
		flash(0);
	}

	//	for (int j = 0; j < samples_i; j++) {
	//		flash3(samples[j] & 1, (samples[j] & (1 << 1)) ? 1 : 0,
	//			   (samples[j] & (1 << 2)) ? 1 : 0,
	//			   (samples[j] & (1 << 4)) ? 1 : 0);
	//	}

	done();
}
