#include <io/enc28j60.h>
#include <io/gpio.h>
#include <io/spi.h>
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

int main() {
	struct enc28j60_controller enc = {0};
	gpio_init(&gpio_pd, onboard_LEDs, SIZEOF_ARR(onboard_LEDs));
	spi_init(&spi_module_1, enc28j60_spi_params);
	enc28j60_init(&enc, &spi_module_1, &enc8j60_spi_slave, true, MAX_FRAME_LEN,
				  1, 1);
	while (1) {
		for (int i = 0; i < SIZEOF_ARR(onboard_LEDs); i++) {
			gpio_write_partial(&gpio_pd, -1, 1 << onboard_LEDs[i].pin);
			delay(LONG_DELAY);
			gpio_write_partial(&gpio_pd, 0, 1 << onboard_LEDs[i].pin);
		}
		for (int i = 0; i < FLASHES; i++) {
			delay(SHORT_DELAY);
			gpio_write_partial(&gpio_pd, -1, LEDS_MASK);
			delay(SHORT_DELAY);
			gpio_write_partial(&gpio_pd, 0, LEDS_MASK);
		}
		delay(SHORT_DELAY);
	}
}
