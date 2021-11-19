#include <io/enc28j60.h>
#include <io/gpio.h>
#include <io/spi.h>
#include <kernel/sched.h>
#include <stdbool.h>

#define SIZEOF_ARR(x) (sizeof(x) / sizeof(*(x)))

#define SHORT_DELAY	  1000
#define LONG_DELAY	  200000
#define DELAY_PORTION 0.99

#define GREEN_LED  12
#define ORANGE_LED 13
#define RED_LED	   14
#define BLUE_LED   15

unsigned int delay_weight = LONG_DELAY;

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

void flash(void *color_idx) {
	const struct gpio_pin *gpio_pin = onboard_LEDs + (uint32_t)color_idx;
	while (gpio_pin->pin != GREEN_LED || delay_weight > SHORT_DELAY) {
		gpio_write_partial(&gpio_pd, -1, 1 << gpio_pin->pin);
		delay(delay_weight);
		gpio_write_partial(&gpio_pd, 0, 1 << gpio_pin->pin);
		sched_yield();
	}
}

int main() {
	struct enc28j60_controller enc = {0};
	gpio_init(&gpio_pd, onboard_LEDs, SIZEOF_ARR(onboard_LEDs));
	spi_init(&spi_module_1, enc28j60_spi_params);
	enc28j60_init(&enc, &spi_module_1, &enc8j60_spi_slave, true, MAX_FRAME_LEN,
				  1, 1);
	sched_init();

	sched_start_task(flash, (void *)0);
	sched_start_task(flash, (void *)1);
	sched_start_task(flash, (void *)2);
	sched_start_task(flash, (void *)3);
	while (1) {
		sched_yield();
		if (delay_weight > SHORT_DELAY) { delay_weight *= DELAY_PORTION; }
	}
}
