/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include "fs.h"
#include "irq.h"
#include "test.h"
#include "stdio.h"
#include "sched.h"
#include "screen.h"
#include "string.h"
#include "common.h"
#include "syscall.h"
#include "smp.h"
#include "mm.h"
#include "mac.h"

#define TASK_INIT (00)

void asm_start();

static void init_memory()
{
}

// Initialize PCB for tests
static void init_pcb()
{
    // initialize queue
    queue_init(&ready_queue);

    // main:
    current_running = &pcb[0];

    // test_scheduler2: task group to test do_scheduler()
    for (int i = 0; i < num_sched2_tasks; i++)
    {
        set_pcb(process_id, &pcb[process_id], sched2_tasks[i]);
        process_id += 1;
    }

    // test_lock2: task group to test lock
    for (int i = 0; i < num_lock_tasks; i++)
    {
        set_pcb(process_id, &pcb[process_id], lock_tasks[i]);
        process_id += 1;
    }

    // test_timer: task group to test clock scheduler
    for (int i = 0; i < num_timer_tasks; i++)
    {
        set_pcb(process_id, &pcb[process_id], timer_tasks[i]);
        process_id += 1;
    }

    // test_cost: task group to test clock scheduler
    for (int i = 0; i < num_cost_tasks; i++)
    {
        set_pcb(process_id, &pcb[process_id], cost_tasks[i]);
        process_id += 1;
    }
}

// Initialize Exception handler
static void init_exception_handler()
{
    // copy exception handler(exception_handler_entry) to entry address
    memcpy((void*) BEV0_EBASE + BEV0_OFFSET, (void *) exception_handler_begin, 
        exception_handler_end - exception_handler_begin);

    // 
}

static void init_exception()
{
    init_exception_handler();
    /* fill nop */

    /* fill nop */

    /* set COUNT & set COMPARE */
    reset_timer(TIMER_INTERVAL);

    /* open interrupt */
}

// [2]
// extern int read_shell_buff(char *buff);

static void init_syscall(void)
{
    syscall[SYSCALL_SLEEP] = (void *) do_sleep;
    syscall[SYSCALL_WRITE] = (void *) screen_write;
    syscall[SYSCALL_CURSOR] = (void *) screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = (void *) screen_reflush;
    syscall[SYSCALL_MUTEX_LOCK_INIT] = (void *) do_mutex_lock_init;
    syscall[SYSCALL_MUTEX_LOCK_ACQUIRE] = (void *) do_mutex_lock_acquire;
    syscall[SYSCALL_MUTEX_LOCK_RELEASE] = (void *) do_mutex_lock_release;
}

/* [0] The beginning of everything >_< */
void __attribute__((section(".entry_function"))) _start(void)
{

    asm_start();

    // init stack space
    init_stack();
    printk("> [INIT] Stack heap initialization succeeded.\n");

    // init interrupt
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n");

    /*
    // init memory
    init_memory();
    printk("> [INIT] Virtual memory initialization succeeded.\n");
    */

    // init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n");

    // init Process Control Block
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n");

    // init screen
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n");

    /*
    // init filesystem
    read_super_block();

    // wake up core1
    loongson3_boot_secondary();
    */

    /* set cp0_status register to allow interrupt */
    // enable exception and interrupt
    // ERL = 0, EXL = 0, IE = 1

    reset_timer(TIMER_INTERVAL);
    // set_cp0_status(initial_cp0_status);

    while (1)
    {
        do_scheduler();
    };
    return;
}
