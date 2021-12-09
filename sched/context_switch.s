.syntax unified

.extern curr_task
.extern replace_curr_task

.macro __disable_irq reg
    mov \reg, #0xef
    msr basepri, \reg
.endm

.macro __enable_irq reg
    mov \reg, #0
    msr basepri, \reg
.endm

/*
 * the absolute offset is too big to encode in an ldr instruction, and
 * `ldr r2, =curr_task` doesn't seem to work, so it is implemented manually.
 */
curr_task_proxy:
    .globl   curr_task_proxy
    .type    curr_task_proxy,%object
    .word    curr_task

context_switch:
    .globl   context_switch
    .type    context_switch,%function
    .fnstart

    stmdb sp!, {r4-r11, r14} @ db - decrement downwards
                             @ !  - save resulting pointer back to sp

    ldr r3, curr_task_proxy @ load proxy
    ldr r2, [r3]            @ dereference proxy

    str sp, [r2]             @ store pointer to top of the stack in the start
                             @ of the current task struct

    stmdb sp!, {r3}
    __disable_irq reg=r0
    dsb
    isb
    bl replace_curr_task
    __enable_irq reg=r0
    ldmia sp!, {r3}


    ldr r2, [r3]            @ dereference proxy
    ldr r0, [r2]            @ load pointer to top of the stack from the start
                            @ of the current task struct

    msr msp, r0
    isb

    ldmia sp!, {r4-r11, r14} @ ia - increment after

    bx r14
    .fnend
