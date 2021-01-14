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
#include "time.h"

#define TASK_INIT (00)

void asm_start();

static void init_memory()
{
    init_TLB();
    page_ctrl_init(&page_ctrl_kernal, KERNEL_PAGE_START, PAGE_SIZE);   // virtual address
    page_ctrl_init(&page_ctrl_user, 0x20000000, PAGE_SIZE);            // physical address
    init_page_table();
}

// Initialize PCB for tests
static void init_pcb()
{
    // initialize queue
    queue_init(&ready_queue);

    // main:
    current_running = &pcb_list[0];
}

// Initialize Exception handler
static void init_exception_handler()
{
    // copy exception handler(exception_handler_entry) to entry address
    memcpy((void*) BEV0_EBASE + BEV0_OFFSET, (void *) exception_handler_begin, 
        exception_handler_end - exception_handler_begin);
    memcpy((void*) BEV0_EBASE, (void *) TLBexception_handler_begin, 
        TLBexception_handler_end - TLBexception_handler_begin);
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
    syscall[SYSCALL_SPAWN] = (void *) do_spawn;
    syscall[SYSCALL_EXIT] = (void *) do_exit;
    syscall[SYSCALL_SLEEP] = (void *) do_sleep;
    syscall[SYSCALL_KILL] = (void *) do_kill;
    syscall[SYSCALL_WAITPID] = (void *) do_waitpid;
    syscall[SYSCALL_PS] = (void *) do_process_show;
    syscall[SYSCALL_GETPID] = (void *) do_getpid;
    syscall[SYSCALL_YIELD] = (void *) do_scheduler;

    syscall[SYSCALL_WRITE] = (void *) screen_write;
    syscall[SYSCALL_CURSOR] = (void *) screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = (void *) screen_reflush;
    syscall[SYSCALL_SCREEN_CLEAR] = (void *) screen_clear;
    syscall[SYSCALL_SERIAL_READ] = (void *) port_read_ch;
    syscall[SYSCALL_SERIAL_WRITE] = (void *) port_write_ch;
    syscall[SYSCALL_MUTEX_LOCK_INIT] = (void *) do_mutex_lock_init;
    syscall[SYSCALL_MUTEX_LOCK_ACQUIRE] = (void *) do_mutex_lock_acquire;
    syscall[SYSCALL_MUTEX_LOCK_RELEASE] = (void *) do_mutex_lock_release;
    syscall[SYSCALL_CONDITION_INIT] = (void *) do_condition_init;
    syscall[SYSCALL_CONDITION_WAIT] = (void *) do_condition_wait;
    syscall[SYSCALL_CONDITION_SIGNAL] = (void *) do_condition_signal;
    syscall[SYSCALL_CONDITION_BROADCAST] = (void *) do_condition_broadcast;
    syscall[SYSCALL_BARRIER_INIT] = (void *) do_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT] = (void *) do_barrier_wait;
    syscall[SYSCALL_BINSEM_GET] = (void *) do_binsem_get;
    syscall[SYSCALL_BINSEM_OP] = (void *) do_binsem_op;
    syscall[SYSCALL_MAILBOX_OPEN] = (void *) do_mbox_open;
    syscall[SYSCALL_MAILBOX_CLOSE] = (void *) do_mbox_close;
    syscall[SYSCALL_MAILBOX_SEND] = (void *) do_mbox_send;
    syscall[SYSCALL_MAILBOX_RECV] = (void *) do_mbox_recv;

    syscall[SYSCALL_GET_TIMER] = (void *) get_timer;
}

static void init_lock()
{
    for (int i = 0; i < NUM_BINSEM; i++)
    {
        do_mutex_lock_init(&binsem_list[i]);
    }
}

static void start_tasks() {
    // test_shell
    for (int i = 0; i < num_shell_tasks; i++)
    {
        do_spawn(shell_tasks[i], 0, NULL);
    }
}

/* [0] The beginning of everything >_< */
void __attribute__((section(".entry_function"))) _start(void)
{

    asm_start();

    // init stack space
    init_stack();
    printk("> [INIT] Stack heap initialization succeeded.\r\n");

    // init interrupt
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\r\n");

    // init memory
    init_memory();
    printk("> [INIT] Virtual memory initialization succeeded.\r\n");

    // init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\r\n");

    // init lock
    init_lock();
    printk("> [INIT] Lock initialized successfully.\r\n");

    // init Process Control Block
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\r\n");

    // init screen
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\r\n");

    // start tasks
    start_tasks();
    printk("> [INIT] TASKs started.\r\n");

    // init mac
    // do_init_mac();
    // printk("> [INIT] MAC initialization succeeded.\r\n");


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
