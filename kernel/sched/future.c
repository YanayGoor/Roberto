#include "internal.h"

#include <kernel/future.h>
#include <kernel/sched.h>

void await(struct future *future) {
	curr_task->state = TASK_WAITING;
	WAITQ_INSERT_TAIL(&future->wait_queue, curr_task);
	sched_yield();
}

void wake_up(struct future *future) {
	struct task *task = WAITQ_FIRST(&future->wait_queue);
	WAITQ_REMOVE(&future->wait_queue, task);
	task->state = TASK_RUNNING;
}