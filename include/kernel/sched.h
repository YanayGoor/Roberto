#ifndef ROBERTO_SCHED_H
#define ROBERTO_SCHED_H

#include <kernel/future.h>
#include <stdint.h>
#include <sys/queue.h>

#define TLIST_HEAD(name, type)		 LIST_HEAD(name, type)
#define TLIST_ENTRY(type)			 LIST_ENTRY(type)
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
	TLIST_ENTRY(task) tasks;
	WAITQ_ENTRY(task) wait_queue;
	enum task_state state;
};

void sched_init(void);
void sched_yield(void);
void sched_start_task(void(function)(void *), void *arg);

#endif // ROBERTO_SCHED_H
