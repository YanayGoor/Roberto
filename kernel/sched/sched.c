#include "internal.h"

#include <stm32f4/stm32f4xx.h>

#include <kernel/sched.h>
#include <stdlib.h>
#include <sys/queue.h>

#define LIST_NEXT_CIRCULAR(head, elm, field)                                   \
	(LIST_NEXT(curr_task, tasks) == NULL ? LIST_FIRST(head)                    \
										 : LIST_NEXT(curr_task, tasks))

LIST_HEAD(, task) tasks = LIST_HEAD_INITIALIZER();
struct task *curr_task = NULL;

static void _free_task(struct task *task) {
	LIST_REMOVE(task, tasks);
	free(task->stack_mem_start);
	free(task);
}

/**
 * Returned to by all tasks except the primary,
 * marks the task as done so it will be freed after the context switch.
 */
static void _finish_task(void) {
	curr_task->state = TASK_DONE;
	sched_yield();
}

/**
 * Choose a new task to run by replacing `curr_task` and do housekeeping (i.e.
 * free finished tasks)
 */
void _sched_replace_curr_task(void) {
	struct task *prev_task = curr_task;
	do {
		curr_task = LIST_NEXT_CIRCULAR(&tasks, curr_task, tasks);
	} while (curr_task->state != TASK_RUNNING);
	if (prev_task->state == TASK_DONE) { _free_task(curr_task); }
}

void sched_init(void) {
	curr_task = calloc(1, sizeof(struct task));
	curr_task->state = TASK_RUNNING;
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
	suspended_stack->lr = (uint32_t)_finish_task;
	suspended_stack->return_addr = (uint32_t)function;
	suspended_stack->r0 = (uint32_t)arg;
	// lsb of function pointer determines if the function is in thumb
	suspended_stack->xpsr = (xPSR_Type){.b = {.T = (uint32_t)function & 1}}.w;

	task->stack_mem_start = stack;
	task->stack_top = (uint8_t *)suspended_stack;
	task->state = TASK_RUNNING;

	LIST_INSERT_HEAD(&tasks, task, tasks);
}
