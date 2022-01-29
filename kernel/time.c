#include <stm32f4/stm32f4xx.h>

#include <kernel/future.h>
#include <kernel/sched.h>
#include <kernel/time.h>
#include <sys/queue.h>

#ifndef TICKS_PER_SEC
#define TICKS_PER_SEC 10000
#endif // TICKS_PER_SEC

#define MS_PER_SEC 1000
#define NS_PER_SEC 1000000000

#define TICKS_TO_NS(ticks) ((uint64_t)ticks * MS_PER_SEC / TICKS_PER_SEC)
#define NS_TO_TICKS(ns)	   ((uint64_t)ns * TICKS_PER_SEC / NS_PER_SEC)

#define STLIST_ENTRY(type)				  LIST_ENTRY(type)
#define STLIST_HEAD(name, type)			  LIST_HEAD(name, type)
#define STLIST_HEAD_INITIALIZER(head)	  LIST_HEAD_INITIALIZER(head)
#define STLIST_FOREACH(var, head)		  LIST_FOREACH(var, head, entry)
#define STLIST_EMPTY(head)				  LIST_EMPTY(head)
#define STLIST_INSERT_HEAD(head, elm)	  LIST_INSERT_HEAD(head, elm, entry)
#define STLIST_REMOVE(elm)				  LIST_REMOVE(elm, entry)
#define STLIST_INSERT_AFTER(listelm, elm) LIST_INSERT_AFTER(listelm, elm, entry)
#define STLIST_INSERT_BEFORE(listelm, elm)                                     \
	LIST_INSERT_BEFORE(listelm, elm, entry)

struct sleeping_task {
	rtime_t wakeup_at;
	struct future future;
	STLIST_ENTRY(sleeping_task) entry;
};

uint64_t ticks = 0;

STLIST_HEAD(, sleeping_task) sleeping_tasks = STLIST_HEAD_INITIALIZER();

static void insert_to_sleeping_tasks(struct sleeping_task *elm) {
	struct sleeping_task *listelm;
	struct sleeping_task *last;

	if (STLIST_EMPTY(&sleeping_tasks)) {
		STLIST_INSERT_HEAD(&sleeping_tasks, elm);
		return;
	}
	STLIST_FOREACH(listelm, &sleeping_tasks) {
		last = listelm;
		if (listelm->wakeup_at > elm->wakeup_at) {
			STLIST_INSERT_BEFORE(listelm, elm);
			return;
		}
	}
	STLIST_INSERT_AFTER(last, elm);
}

static void __attribute__((noreturn)) wakeup_sleeping_tasks() {
	while (1) {
		rtime_t time = get_time();
		struct sleeping_task *entry;
		STLIST_FOREACH(entry, &sleeping_tasks) {
			if (entry->wakeup_at > time) { break; }
			wake_up(&entry->future);
		}
		sched_yield();
	}
}

void SysTick_Handler(void) {
	ticks++;
}

void time_init(void) {
	SysTick_Config(SystemCoreClock / TICKS_PER_SEC);
	sched_start_task(wakeup_sleeping_tasks, NULL);
}

rtime_t get_time(void) {
	return TICKS_TO_NS(ticks);
}

void nsleep(unsigned int nanoseconds) {
	uint64_t target = ticks + NS_TO_TICKS(nanoseconds);
	while (ticks < target) {
		__NOP();
	}
}

void msleep(unsigned int milliseconds) {
	struct sleeping_task *task = malloc(sizeof(struct sleeping_task));
	task->wakeup_at = get_time() + milliseconds;
	task->future = FUTURE_INITIALIZER(&task->future);
	insert_to_sleeping_tasks(task);

	await(&task->future);

	STLIST_REMOVE(task);
	free(task);
}

void sleep(unsigned int seconds) {
	msleep(seconds * MS_PER_SEC);
}