#include "internal.h"

#include <kernel/future.h>
#include <kernel/sched.h>
#include <sys/queue.h>

void await(struct future *future) {
	curr_task->state = TASK_WAITING;
	TAILQ_INSERT_TAIL(&future->wait_queue, curr_task, wait_queue);
	sched_yield();
}

void wake_up(struct future *future) {
	struct task *task = TAILQ_FIRST(&future->wait_queue);
	TAILQ_REMOVE(&future->wait_queue, task, wait_queue);
	task->state = TASK_RUNNING;
}