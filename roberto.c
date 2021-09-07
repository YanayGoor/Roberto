#include <stm32f4/stm32f4xx.h>

#define SET_BIT_INX(reg, bit_inx)	reg |= 1 << bit_inx;
#define CLEAR_BIT_INX(reg, bit_inx) reg &= ~(1 << bit_inx);

#define SHORT_DELAY 800000
#define LONG_DELAY	1600000

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

void delay(int weight) {
	for (int i = 0; i < weight; i++) {}
}

int main() {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 |
					GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
	while (1) {
		for (int inx = GREEN_LED; inx <= BLUE_LED; inx++) {
			SET_BIT_INX(GPIOD->ODR, inx);
			delay(LONG_DELAY);
			CLEAR_BIT_INX(GPIOD->ODR, inx);
		}
		for (int i = 0; i < 2; i++) {
			delay(SHORT_DELAY);
			GPIOD->ODR |= LEDS_MASK;
			delay(SHORT_DELAY);
			GPIOD->ODR &= ~LEDS_MASK;
		}
		delay(SHORT_DELAY);
	}
}