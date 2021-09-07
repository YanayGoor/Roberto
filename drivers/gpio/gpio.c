#include <stm32f4/stm32f4xx.h>

#include <io/gpio.h>
#include <limits.h>
#include <stddef.h>

#define SIZEOF_BITS(x) (sizeof(x) * CHAR_BIT)

#define SET_MASK(value, mask)	(value & mask)
#define CLEAR_MASK(value, mask) (~(value)&mask)

#define MODE_MASK	((1 << GPIO_MODE_BITLEN) - 1)
#define ALT_FN_MASK ((1 << GPIO_ALT_FN_BITLEN) - 1)

static void enable_port_clock(const struct gpio_port *port) {
	RCC->AHB1ENR |= port->enr;
}

static void set_pins_mode(const struct gpio_port *port, uint32_t set_mask,
						  uint32_t clear_mask) {
	MODIFY_REG(port->regs->MODER, clear_mask, set_mask);
}

static void set_pins_alt_fn(const struct gpio_port *port, uint64_t set_mask,
							uint64_t clear_mask) {
	MODIFY_REG(port->regs->AFR[0], clear_mask, set_mask);
	MODIFY_REG(port->regs->AFR[1], clear_mask >> SIZEOF_BITS(uint32_t),
			   set_mask >> SIZEOF_BITS(uint32_t));
}

static void gpio_init_pin(const struct gpio_port *port, struct gpio_pin pin) {
	enable_port_clock(port);
	set_pins_mode(
		port, SET_MASK(pin.mode, MODE_MASK) << (pin.pin * GPIO_MODE_BITLEN),
		CLEAR_MASK(pin.mode, MODE_MASK) << (pin.pin * GPIO_MODE_BITLEN));
	set_pins_alt_fn(
		port, SET_MASK(pin.alt_fn, ALT_FN_MASK) << (pin.pin * GPIO_MODE_BITLEN),
		CLEAR_MASK(pin.alt_fn, ALT_FN_MASK) << (pin.pin * GPIO_MODE_BITLEN));
}

void gpio_init_bulk(const struct gpio_port *port, uint16_t pins,
					enum gpio_mode mode) {
	uint32_t set_mask = 0;
	uint32_t clear_mask = 0;
	for (int i = 0; i < SIZEOF_BITS(pins); i++) {
		if (pins & (1 << i)) {
			set_mask |= SET_MASK(mode, MODE_MASK) << (i * GPIO_MODE_BITLEN);
			clear_mask |= CLEAR_MASK(mode, MODE_MASK) << (i * GPIO_MODE_BITLEN);
		}
	}
	enable_port_clock(port);
	set_pins_mode(port, set_mask, clear_mask);
}

void gpio_init(const struct gpio_port *port, struct gpio_pin const *pins,
			   size_t pins_len) {
	for (int i = 0; i < pins_len; i++) {
		gpio_init_pin(port, pins[i]);
	}
}

#define GPIO_DEFINE_PORT(lower, upper)                                         \
	const struct gpio_port gpio_p##lower = {.regs = (GPIO##upper),             \
											.enr =                             \
												RCC_AHB1ENR_GPIO##upper##EN}

GPIO_DEFINE_PORT(a, A);
GPIO_DEFINE_PORT(b, B);
GPIO_DEFINE_PORT(c, C);
GPIO_DEFINE_PORT(d, D);
GPIO_DEFINE_PORT(e, E);
GPIO_DEFINE_PORT(f, F);
GPIO_DEFINE_PORT(g, G);
GPIO_DEFINE_PORT(h, H);
GPIO_DEFINE_PORT(i, I);