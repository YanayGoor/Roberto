#include <stm32f4/stm32f4xx.h>

#include <io/gpio.h>

#define SET_BIT_INX(reg, bit_inx)	reg |= 1 << bit_inx;
#define CLEAR_BIT_INX(reg, bit_inx) reg &= ~(1 << bit_inx);

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

const struct gpio_pin onboard_LEDs[] = {
	{.port = GPIO_PD, .pin = GREEN_LED, .mode = GPIO_OUTPUT},
	{.port = GPIO_PD, .pin = ORANGE_LED, .mode = GPIO_OUTPUT},
	{.port = GPIO_PD, .pin = RED_LED, .mode = GPIO_OUTPUT},
	{.port = GPIO_PD, .pin = BLUE_LED, .mode = GPIO_OUTPUT},
};

void delay(int weight) {
	for (int i = 0; i < weight; i++) {}
}

int main() {
#ifdef BULK
	gpio_init_bulk(GPIO_PD, LEDS_MASK, GPIO_OUTPUT);
#else
	gpio_init(onboard_LEDs, SIZEOF_ARR(onboard_LEDs));
#endif
	while (1) {
		for (int i = 0; i < SIZEOF_ARR(onboard_LEDs); i++) {
			SET_BIT_INX(GPIOD->ODR, onboard_LEDs[i].pin);
			delay(LONG_DELAY);
			CLEAR_BIT_INX(GPIOD->ODR, onboard_LEDs[i].pin);
		}
		for (int i = 0; i < FLASHES; i++) {
			delay(SHORT_DELAY);
			GPIOD->ODR |= LEDS_MASK;
			delay(SHORT_DELAY);
			GPIOD->ODR &= ~LEDS_MASK;
		}
		delay(SHORT_DELAY);
	}
}