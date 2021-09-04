#include <stm32f4/stm32f4xx.h>

#define SET_BIT_INX(reg, bit_inx) reg |= 1 << bit_inx;
#define CLEAR_BIT_INX(reg, bit_inx) reg &= ~(1 << bit_inx);

void delay(int weight) {
    for (int i = 0; i < weight * 100000; i++) {}
}

int main() {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
    while (1) {
        for (int inx = 12; inx <= 15; inx++) {
            SET_BIT_INX(GPIOD->ODR, inx);
            delay(10);
            CLEAR_BIT_INX(GPIOD->ODR, inx);
        }
        for (int i = 0; i < 2; i++) {
            delay(5);
            GPIOD->ODR |= 0xf << 12;
            delay(5);
            GPIOD->ODR &= ~(0xf << 12);
        }
        delay(5);
    }
}