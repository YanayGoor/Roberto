#include "internal.h"

#include <kernel/future.h>
#include <kernel/sched.h>

void await(struct future *future) {
	if (future->done) { return; }
	curr_task->state = TASK_WAITING;
	future->waiting = curr_task;
	sched_yield();
}

void awaitq(waitq_head *wait_queue) {
	curr_task->state = TASK_WAITING;
	WAITQ_INSERT_TAIL(wait_queue, curr_task);
	sched_yield();
}

void wake_up(struct future *future) {
	future->done = true;
	if (future->waiting == NULL) { return; }
	future->waiting->state = TASK_RUNNING;
	future->waiting = NULL;
}

void wake_up_one(waitq_head *wait_queue) {
	struct task *task = WAITQ_FIRST(wait_queue);
	if (task == NULL) { return; }
	WAITQ_REMOVE(wait_queue, task);
	task->state = TASK_RUNNING;
}

void wake_up_all(waitq_head *wait_queue) {
	struct task *task;
	while ((task = WAITQ_FIRST(wait_queue))) {
		task->state = TASK_RUNNING;
		WAITQ_REMOVE(wait_queue, task);
	}
}
