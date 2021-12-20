#ifndef SCHED_INTERNAL_H
#define SCHED_INTERNAL_H

#include <stdint.h>
#include <sys/queue.h>

#define KB		  1024
#define STACK_KBS 4

#define EXC_RETURN_BASE	   0xffffffe1
#define EXC_RETURN_MSP	   EXC_RETURN_BASE
#define EXC_RETURN_PSP	   EXC_RETURN_BASE | (1 << 2)
#define EXC_RETURN_HANDLER EXC_RETURN_BASE
#define EXC_RETURN_THREAD  EXC_RETURN_BASE | (1 << 3)
#define EXC_RETURN_FPC_ON  EXC_RETURN_BASE
#define EXC_RETURN_FPC_OFF EXC_RETURN_BASE | (1 << 4)

struct suspended_task_stack {
	/* pushed in context_switch() */
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t suspended_at_lr;

	/* pushed by hardware */
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t return_addr;
	uint32_t xpsr;
};

#endif // SCHED_INTERNAL_H
