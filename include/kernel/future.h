#ifndef ROBERTO_FUTURE_H
#define ROBERTO_FUTURE_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>

typedef TAILQ_HEAD(, task) waitq_head;
typedef TAILQ_ENTRY(task) waitq_entry;

#define WAITQ_INIT(head)			 TAILQ_INIT(head)
#define WAITQ_FIRST(head)			 TAILQ_FIRST(head)
#define WAITQ_NEXT(elm)				 TAILQ_NEXT(elm, wait_queue)
#define WAITQ_INSERT_TAIL(head, elm) TAILQ_INSERT_TAIL(head, elm, wait_queue)
#define WAITQ_REMOVE(head, elm)		 TAILQ_REMOVE(head, elm, wait_queue)
#define WAITQ_INITIALIZER(head)		 TAILQ_HEAD_INITIALIZER(head)

struct future {
	struct task *waiting;
	bool done;
};

#define FUTURE_INITIALIZER(_future)                                            \
	(struct future) {                                                          \
		.waiting = NULL, .done = false,                                        \
	}

void await(struct future *future);
void wake_up(struct future *future);

void awaitq(waitq_head *wait_queue);
void wake_up_one(waitq_head *wait_queue);
void wake_up_all(waitq_head *wait_queue);

#endif // ROBERTO_FUTURE_H
