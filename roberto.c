#include <io/gpio.h>
#include <kernel/sched.h>

#define SIZEOF_ARR(x) (sizeof(x) / sizeof(*(x)))

#define SHORT_DELAY	  1000
#define LONG_DELAY	  200000
#define DELAY_PORTION 0.99

#define GREEN_LED  12
#define ORANGE_LED 13
#define RED_LED	   14
#define BLUE_LED   15

unsigned int delay_weight = LONG_DELAY;

const struct gpio_pin onboard_LEDs[] = {
	{.pin = GREEN_LED, .mode = GPIO_OUTPUT},
	{.pin = ORANGE_LED, .mode = GPIO_OUTPUT},
	{.pin = RED_LED, .mode = GPIO_OUTPUT},
	{.pin = BLUE_LED, .mode = GPIO_OUTPUT},
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
#ifdef BULK
	gpio_init_bulk(&gpio_pd, LEDS_MASK, GPIO_OUTPUT);
#else
	gpio_init(&gpio_pd, onboard_LEDs, SIZEOF_ARR(onboard_LEDs));
#endif
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