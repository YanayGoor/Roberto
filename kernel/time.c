#include <stm32f4/stm32f4xx.h>

#include <kernel/future.h>
#include <sys/queue.h>

#define TICKS_PER_SEC 10000
#define MS_PER_SEC	  1000

struct sleeping_task {
	unsigned int wakeup_at;
	struct future future;
	LIST_ENTRY(sleeping_task) entry;
};

unsigned int curr_seconds = 0;
unsigned int curr_ticks = 0;

LIST_HEAD(, sleeping_task) sleeping_tasks = LIST_HEAD_INITIALIZER();

unsigned int get_time(void);

static void wakeup_sleeping() {
	unsigned int time = get_time();
	struct sleeping_task *entry;
	LIST_FOREACH(entry, &sleeping_tasks, entry) {
		if (entry->wakeup_at > time) { break; }
		wake_up(&entry->future);
	}
}

void SysTick_Handler(void) {
	curr_ticks++;
	if (curr_ticks == TICKS_PER_SEC) {
		curr_ticks = 0;
		curr_seconds++;
	}
	wakeup_sleeping();
}

void time_init(void) {
	SysTick_Config(SystemCoreClock / TICKS_PER_SEC);
}

unsigned int get_time(void) {
	return curr_seconds * MS_PER_SEC + curr_ticks * MS_PER_SEC / TICKS_PER_SEC;
}

void sleep(unsigned int seconds) {
	struct sleeping_task *task = malloc(sizeof(struct sleeping_task));
	FUTURE_INIT(&task->future);
	task->wakeup_at = get_time() + seconds * MS_PER_SEC;
	if (LIST_EMPTY(&sleeping_tasks)) {
		LIST_INSERT_HEAD(&sleeping_tasks, task, entry);
	} else {
		struct sleeping_task *entry;
		LIST_FOREACH(entry, &sleeping_tasks, entry) {
			if (LIST_NEXT(entry, entry) == NULL ||
				LIST_NEXT(entry, entry)->wakeup_at < task->wakeup_at) {
				LIST_INSERT_AFTER(entry, task, entry);
				break;
			}
		}
	}

	await(&task->future);

	LIST_REMOVE(task, entry);
}