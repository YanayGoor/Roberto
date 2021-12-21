#ifndef ROBERTO_FUTURE_H
#define ROBERTO_FUTURE_H

#include <stdlib.h>
#include <sys/queue.h>

struct future {
	TAILQ_HEAD(, task) wait_queue;
};

#define FUTURE_INIT(future) TAILQ_INIT(&(future)->wait_queue)

void await(struct future *future);
void wake_up(struct future *future);

#endif // ROBERTO_FUTURE_H
