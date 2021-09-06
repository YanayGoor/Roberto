#include <stm32f4/stm32f4xx.h>

#include <io/gpio.h>
#include <limits.h>
#include <stddef.h>

#define SIZEOF_BITS(x) (sizeof(x) * CHAR_BIT)

#define SET_MASK(value, mask)	(value & mask)
#define CLEAR_MASK(value, mask) (~(value)&mask)

#define MODE_MASK	((1 << GPIO_MODE_BITLEN) - 1)
#define ALT_FN_MASK ((1 << GPIO_ALT_FN_BITLEN) - 1)

static GPIO_TypeDef *const port_regs[] = {
	[GPIO_A] = GPIOA, [GPIO_B] = GPIOB, [GPIO_C] = GPIOC,
	[GPIO_D] = GPIOD, [GPIO_E] = GPIOE,
};

static uint32_t const rcc_en[] = {
	[GPIO_A] = RCC_AHB1ENR_GPIOAEN, [GPIO_B] = RCC_AHB1ENR_GPIOBEN,
	[GPIO_C] = RCC_AHB1ENR_GPIOCEN, [GPIO_D] = RCC_AHB1ENR_GPIODEN,
	[GPIO_E] = RCC_AHB1ENR_GPIOEEN,
};

static void enable_port_clock(enum gpio_port port) {
	RCC->AHB1ENR |= rcc_en[port];
}

static void set_pins_mode(enum gpio_port port, uint32_t set_mask,
						  uint32_t clear_mask) {
	MODIFY_REG(port_regs[port]->MODER, clear_mask, set_mask);
}

static void set_pins_alt_fn(enum gpio_port port, uint64_t set_mask,
							uint64_t clear_mask) {
	MODIFY_REG(port_regs[port]->AFR[0], clear_mask, set_mask);
	MODIFY_REG(port_regs[port]->AFR[1], clear_mask >> SIZEOF_BITS(uint32_t),
			   set_mask >> SIZEOF_BITS(uint32_t));
}

static void gpio_init_pin(struct gpio_pin pin) {
	enable_port_clock(pin.port);
	set_pins_mode(
		pin.port, SET_MASK(pin.mode, MODE_MASK) << (pin.pin * GPIO_MODE_BITLEN),
		CLEAR_MASK(pin.mode, MODE_MASK) << (pin.pin * GPIO_MODE_BITLEN));
	set_pins_alt_fn(
		pin.port,
		SET_MASK(pin.alt_fn, ALT_FN_MASK) << (pin.pin * GPIO_MODE_BITLEN),
		CLEAR_MASK(pin.alt_fn, ALT_FN_MASK) << (pin.pin * GPIO_MODE_BITLEN));
}

void gpio_init_bulk(enum gpio_port port, uint16_t pins, enum gpio_mode mode) {
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

void gpio_init(struct gpio_pin const *pins, size_t pins_len) {
	for (int i = 0; i < pins_len; i++) {
		gpio_init_pin(pins[i]);
	}
}