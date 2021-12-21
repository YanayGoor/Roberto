#include "internal.h"

#include <kernel/future.h>
#include <kernel/sched.h>
#include <stdlib.h>
#include <sys/queue.h>

void await(struct future *future) {
	TAILQ_INSERT_TAIL(&future->wait_queue, curr_task, wait_queue);
	curr_task->state = TASK_WAITING;
	sched_yield();
	// TODO: cleanup future?
}

void wake_up(struct future *future) {
	struct task *task = TAILQ_FIRST(&future->wait_queue);
	TAILQ_REMOVE(&future->wait_queue, task, wait_queue);
	task->state = TASK_RUNNING;
}

// await(spi_write_buff(...))