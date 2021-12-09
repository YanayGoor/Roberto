#include "stm32f4/stm32f4xx.h"

#include <memory.h>
#include <sched/task.h>
#include <stdlib.h>
#include <sys/queue.h>

LIST_HEAD(tasks_head, task) tasks = LIST_HEAD_INITIALIZER(tasks);

#define GREEN_LED		12
#define ORANGE_LED		13
#define RED_LED			14
#define BLUE_LED		15
#define GREEN_LED_MASK	(1 << GREEN_LED)
#define ORANGE_LED_MASK (1 << ORANGE_LED)
#define RED_LED_MASK	(1 << RED_LED)
#define BLUE_LED_MASK	(1 << BLUE_LED)
#define LEDS_MASK                                                              \
	(GREEN_LED_MASK | ORANGE_LED_MASK | RED_LED_MASK | BLUE_LED_MASK)

struct task *curr_task = NULL;

extern void context_switch(void);

void PendSV_Handler(void) {
	context_switch();
}

void sched_init(void) {
	curr_task = calloc(1, sizeof(struct task));
	LIST_INSERT_HEAD(&tasks, curr_task, tasks);
}

void sched_yield(void) {
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}
