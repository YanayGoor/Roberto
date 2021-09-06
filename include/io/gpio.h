#ifndef GPIO_H
#define GPIO_H

#include <stm32f4/stm32f4xx.h>

#include <errno.h>
#include <stddef.h>

#define GPIO_MODE_BITLEN   2
#define GPIO_ALT_FN_BITLEN 4

enum gpio_port {
	GPIO_PA,
	GPIO_PB,
	GPIO_PC,
	GPIO_PD,
	GPIO_PE,
	GPIO_PF,
	GPIO_PG,
	GPIO_PH,
	GPIO_PI
};

enum gpio_mode {
	GPIO_INPUT = 0b00,
	GPIO_OUTPUT = 0b01,
	GPIO_ALTERNATE = 0b10,
	GPIO_ANALOG = 0b11,
};

struct gpio_pin {
	enum gpio_port port;
	uint8_t pin : 4;
	enum gpio_mode mode;
	uint8_t alt_fn : GPIO_ALT_FN_BITLEN;
};

void gpio_init_bulk(enum gpio_port port, uint16_t pins, enum gpio_mode mode);
void gpio_init(struct gpio_pin const *pins, size_t pins_len);

#endif /* GPIO_H */
