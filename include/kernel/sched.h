#ifndef ROBERTO_SCHED_H
#define ROBERTO_SCHED_H

#include <kernel/future.h>
#include <stdint.h>
#include <sys/queue.h>

typedef LIST_HEAD(, task) tlist_head;
typedef LIST_ENTRY(task) tlist_entry;

#define TLIST_HEAD_INITIALIZER(head) LIST_HEAD_INITIALIZER(head)
#define TLIST_FIRST(head)			 LIST_FIRST(head)
#define TLIST_INSERT_HEAD(head, elm) LIST_INSERT_HEAD(head, elm, tasks)
#define TLIST_REMOVE(elm)			 LIST_REMOVE(elm, tasks)
#define TLIST_NEXT(head, elm)                                                  \
	(LIST_NEXT(elm, tasks) == NULL ? LIST_FIRST(head) : LIST_NEXT(elm, tasks))

enum task_state { TASK_RUNNING, TASK_DONE, TASK_WAITING };

struct task {
	uint8_t *stack_top;
	uint8_t *stack_mem_start;
	tlist_entry tasks;
	waitq_entry wait_queue;
	enum task_state state;
};

void sched_init(void);
void sched_yield(void);
void sched_start_task(void(function)(void *), void *arg);

#endif // ROBERTO_SCHED_H
