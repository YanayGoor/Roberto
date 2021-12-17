#include "internal.h"

#include <stm32f4/stm32f4xx.h>

#include <kernel/sched.h>
#include <stdlib.h>
#include <sys/queue.h>

LIST_HEAD(, task) tasks = LIST_HEAD_INITIALIZER(tasks);
struct task *curr_task = NULL;

extern void context_switch(void);

void _sched_replace_curr_task(void) {
	curr_task = LIST_NEXT(curr_task, tasks);
	if (curr_task == NULL) { curr_task = LIST_FIRST(&tasks); }
}

void sched_init(void) {
	curr_task = calloc(1, sizeof(struct task));
	LIST_INSERT_HEAD(&tasks, curr_task, tasks);
}

void sched_yield(void) {
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

void sched_start_task(void(function)(void *), void *arg) {
	struct suspended_task_stack *suspended_stack;
	struct task *task = calloc(1, sizeof(struct task));
	uint8_t *stack = calloc(STACK_KBS, KB);

	suspended_stack =
		(struct suspended_task_stack *)(stack + (KB * STACK_KBS) -
										sizeof(struct suspended_task_stack));
	suspended_stack->suspended_at_lr =
		EXC_RETURN_MSP | EXC_RETURN_THREAD | EXC_RETURN_FPC_OFF;

	suspended_stack->return_addr = (uint32_t)function;
	suspended_stack->r0 = (uint32_t)arg;
	// lsb of function pointer determines if the function is in thumb
	suspended_stack->xpsr = (xPSR_Type){.b = {.T = (uint32_t)function & 1}}.w;

	task->stack_mem_start = stack;
	task->stack_top = (uint8_t *)suspended_stack;
	LIST_INSERT_HEAD(&tasks, task, tasks);
}
