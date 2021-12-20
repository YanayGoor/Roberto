#ifndef GPIO_H
#define GPIO_H

#include <stm32f4/stm32f4xx.h>

#include <errno.h>
#include <stddef.h>

#define GPIO_MODE_BITLEN   2
#define GPIO_ALT_FN_BITLEN 4

struct gpio_port {
	GPIO_TypeDef *regs;
	uint32_t enr;
};

enum gpio_mode {
	GPIO_INPUT = 0b00,
	GPIO_OUTPUT = 0b01,
	GPIO_ALTERNATE = 0b10,
	GPIO_ANALOG = 0b11,
};

struct gpio_pin {
	uint8_t pin;
	enum gpio_mode mode;
	uint8_t alt_fn;
};

const struct gpio_port gpio_pa;
const struct gpio_port gpio_pb;
const struct gpio_port gpio_pc;
const struct gpio_port gpio_pd;
const struct gpio_port gpio_pe;
const struct gpio_port gpio_pf;
const struct gpio_port gpio_pg;
const struct gpio_port gpio_ph;
const struct gpio_port gpio_pi;

void gpio_init_pin(const struct gpio_port *port, struct gpio_pin pin);
void gpio_init_bulk(const struct gpio_port *port, uint16_t pins,
					enum gpio_mode mode);
void gpio_init(const struct gpio_port *port, struct gpio_pin const *pins,
			   size_t pins_len);
void gpio_write_partial(const struct gpio_port *port, uint32_t value,
						uint32_t mask);
void gpio_write(const struct gpio_port *port, uint32_t value);
uint32_t gpio_read(const struct gpio_port *port);

#endif /* GPIO_H */
