#include "kernel/time.h"

#include <io/enc28j60.h>
#include <io/gpio.h>
#include <io/spi.h>
#include <kernel/sched.h>
#include <malloc.h>
#include <memory.h>
#include <stdbool.h>

#define SIZEOF_ARR(x) (sizeof(x) / sizeof(*(x)))

#define SHORT_DELAY	  10000
#define LONG_DELAY	  2000000
#define DELAY_PORTION 0.99

#define GREEN_LED  12
#define ORANGE_LED 13
#define RED_LED	   14
#define BLUE_LED   15

unsigned int delay_weight = LONG_DELAY;

#define MAX_FRAME_LEN 1518
#define ETHERNOT_LEN  6

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
											   .baud_rate = 5};

const struct spi_slave enc8j60_spi_slave = {
	.ss_port = &gpio_pb,
	.ss_pin = 6,
};

void delay(int weight) {
	for (int i = 0; i < weight; i++) {}
}

void turn_led_on(struct gpio_pin led) {
	gpio_write_partial(&gpio_pd, -1, 1 << led.pin);
}

void turn_led_off(struct gpio_pin led) {
	gpio_write_partial(&gpio_pd, 0, 1 << led.pin);
}

// void flash(void *color_idx) {
//	const struct gpio_pin gpio_pin = onboard_LEDs[(uint32_t)color_idx];
//	while (gpio_pin.pin != GREEN_LED || delay_weight > SHORT_DELAY) {
//		turn_led_on(gpio_pin);
//		delay(delay_weight);
//		turn_led_off(gpio_pin);
//		sched_yield();
//	}
//}
void flash_sleep(void *color_idx) {
	const struct gpio_pin gpio_pin = onboard_LEDs[(uint32_t)color_idx];
	while (1) {
		turn_led_on(gpio_pin);
		sleep(2);
		turn_led_off(gpio_pin);
		sleep(2);
	}
}

const uint8_t src_mac[ETHERNOT_LEN] = {1, 3, 3, 7, 9, 9};

void enc_poll(void *enc_arg) {
	int bytes_read;
	unsigned int packets_read = 0;
	struct enc28j60_controller *enc = enc_arg;
	struct enc28j60_pkt_rx_hdr *hdr =
		malloc(sizeof(struct enc28j60_pkt_rx_hdr));
	uint8_t *buff = malloc(MAX_FRAME_LEN);

	while (1) {
		sched_yield();
		bool rx_err = enc28j60_get_errors(enc) & ENC28J60_RX_ERR;
		uint8_t packets_received = enc28j60_packets_received(enc);
		if (rx_err) { turn_led_on(onboard_LEDs[2]); }
		if (!packets_received) {
			turn_led_on(onboard_LEDs[0]);
			continue;
		}
		turn_led_off(onboard_LEDs[0]);
		turn_led_on(onboard_LEDs[1]);
		bytes_read =
			enc28j60_receive_packet(enc, hdr, buff, enc->max_frame_length);
		turn_led_off(onboard_LEDs[1]);
		if (bytes_read == -1) {
			turn_led_on(onboard_LEDs[3]);
			continue;
		}
		turn_led_off(onboard_LEDs[3]);
		turn_led_off(onboard_LEDs[2]);
		packets_read++;

		if (!enc28j60_get_tx_busy(enc)) {
			memcpy(buff, buff + ETHERNOT_LEN, ETHERNOT_LEN);
			memcpy(buff + ETHERNOT_LEN, src_mac, ETHERNOT_LEN);
			enc28j60_transmit_packet(enc, (uint8_t *)buff, hdr->byte_count,
									 ENC28J60_PCRCEN);
		}
	}
}

int main() {
	struct enc28j60_controller *enc =
		malloc(sizeof(struct enc28j60_controller));

	gpio_init(&gpio_pd, onboard_LEDs, SIZEOF_ARR(onboard_LEDs));
	spi_init(&spi_module_1, enc28j60_spi_params);

	// TODO: separate init to struct and hardware or automatically reset
	//	enc28j60_init(enc, &spi_module_1, &enc8j60_spi_slave, true,
	// MAX_FRAME_LEN, 				  1, 1); 	enc28j60_reset(enc);
	// delay(LONG_DELAY); 	enc28j60_init(enc, &spi_module_1,
	// &enc8j60_spi_slave,
	// true, MAX_FRAME_LEN, 				  1, 1);
	sched_init();
	time_init();

	sched_start_task(flash_sleep, (void *)0);
	sched_start_task(flash_sleep, (void *)1);
	sched_start_task(flash_sleep, (void *)2);
	sched_start_task(flash_sleep, (void *)3);
	//	sched_start_task(enc_poll, enc);

	while (1) {
		sched_yield();
		if (delay_weight > SHORT_DELAY) { delay_weight *= DELAY_PORTION; }
	}
}
