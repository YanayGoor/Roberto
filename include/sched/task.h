#ifndef ROBERTO_TASK_H
#define ROBERTO_TASK_H

#include <stdint.h>
#include <sys/queue.h>

struct task {
	uint8_t *stack_top;
	uint8_t *stack_mem_start;
	LIST_ENTRY(task) tasks;
};

void sched_init(void);

void sched_yield(void);

#endif // ROBERTO_TASK_H
