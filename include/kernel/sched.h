#ifndef ROBERTO_SCHED_H
#define ROBERTO_SCHED_H

#include <stdint.h>
#include <sys/queue.h>

enum task_state { TASK_RUNNING, TASK_DONE, TASK_WAITING };

struct task {
	uint8_t *stack_top;
	uint8_t *stack_mem_start;
	LIST_ENTRY(task) tasks;
	TAILQ_ENTRY(task) wait_queue;
	enum task_state state;
};

void sched_init(void);
void sched_yield(void);
void sched_start_task(void(function)(void *), void *arg);

#endif // ROBERTO_SCHED_H
