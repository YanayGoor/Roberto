#ifndef ROBERTO_FUTURE_H
#define ROBERTO_FUTURE_H

#include <stdlib.h>
#include <sys/queue.h>

#define WAITQ_HEAD(name, type)		 TAILQ_HEAD(name, type)
#define WAITQ_ENTRY(type)			 TAILQ_ENTRY(type)
#define WAITQ_INIT(head)			 TAILQ_INIT(head)
#define WAITQ_FIRST(head)			 TAILQ_FIRST(head)
#define WAITQ_INSERT_TAIL(head, elm) TAILQ_INSERT_TAIL(head, elm, wait_queue)
#define WAITQ_REMOVE(head, elm)		 TAILQ_REMOVE(head, elm, wait_queue)
#define WAITQ_INITIALIZER(head)		 TAILQ_HEAD_INITIALIZER(head)

struct future {
	WAITQ_HEAD(, task) wait_queue;
};

#define FUTURE_INITIALIZER(_future)                                            \
	(struct future) {                                                          \
		WAITQ_INITIALIZER((_future)->wait_queue)                               \
	}

void await(struct future *future);
void wake_up(struct future *future);

#endif // ROBERTO_FUTURE_H
