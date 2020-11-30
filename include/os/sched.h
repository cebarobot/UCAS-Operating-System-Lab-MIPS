/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking, 
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include "type.h"
#include "queue.h"
#include "lock.h"

// #define PRIORITY_SCHED

#define NUM_MAX_TASK 32
#define CORE_NUM 2
#define TASK_NAME_LEN 32
#define MAX_LOCK 10

// ! This Part is strong related with architecture
#define STACK_TOP       0xffffffffa0f00000
#define USER_STACK_TOP  0xffffffffa0e00000
#define STACK_SIZE      0x1000

/**
 * used to save register infomation 
 * ! This Part is strong related with architecture
 */
typedef struct regs_context
{
    // main processer registers
    reg_t regs[32];
    // cp0 registers
    reg_t cp0_status;
    reg_t cp0_cause;
    reg_t hi;
    reg_t lo;
    reg_t badvaddr;
    reg_t epc;
    reg_t pc;

} regs_context_t; /* 256 + 56 = 312B */

typedef enum
{
    TASK_NONE,
    TASK_SLEEPING,
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
} task_status_t;

extern char task_status_name[][10];

typedef enum
{
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/**
 * Process Control Block
 * ! This Part is strong related with architecture
 */
typedef struct pcb
{
    // stack pointer
    reg_t kernel_sp;
    reg_t user_sp;
    // // register context
    // regs_context_t kernel_context;
    // regs_context_t user_context;

    // previous, next pointer for queue
    void *prev;
    void *next;

    // task in which queue
    queue_t *in_queue;

    // priority
    int64_t priority;

    // last run time
    int64_t last_run;

    // name
    char name[TASK_NAME_LEN];

    // process id
    pid_t pid;

    // task type: kernel/user thread/process
    task_type_t type;

    // task status: SLEEPING | BLOCK | READY | RUNNING
    task_status_t status;

    uint32_t sleep_until;
    pid_t waitpid;

    // locks
    // TODO need to improve 
    mutex_lock_t * locks_got[MAX_LOCK];

    /* cursor position */
    uint32_t cursor_x;
    uint32_t cursor_y;

} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    char name[TASK_NAME_LEN];
    uint64_t entry_point;
    task_type_t type;
    int64_t priority;
} task_info_t;

// ready queue to run 
extern queue_t ready_queue;

// sleep queue 
extern queue_t sleep_queue;

// waitpid queue
extern queue_t waitpid_queue;

/* current running task PCB */
extern pcb_t *current_running;
extern pid_t process_id;

extern pcb_t pcb_list[NUM_MAX_TASK];
extern uint32_t initial_cp0_status;

void do_scheduler(void);

int do_spawn(task_info_t *);
void do_exit(void);
void do_sleep(uint32_t);

int do_kill(pid_t pid);
int do_waitpid(pid_t pid);

void save_cursor();
void restore_cursor();

/**
 * Block current running task into the specific block queue
 * @param queue The block queue
 */
void do_block(queue_t *queue);

/**
 * Unblock one task in the specific block queue
 * @param queue The block queue
 */
void do_unblock_one(queue_t *queue);

/**
 * Unblock all tasks in the specific block queue
 * @param queue The block queue
 */
void do_unblock_all(queue_t *queue);


/**
 * Initialize stack and heap space
 */
void init_stack();

/**
 * Allocate kernel stack memory for one task
 * @return stack address which is allocated
 */
uint64_t new_kernel_stack();

/**
 * Allocate user stack memory for one task
 * @return stack address which is allocated
 */
uint64_t new_user_stack();

/**
 * Free kernel stack memory for one task
 * @param stack_addr stack address which will be freed
 */
static void free_kernel_stack(uint64_t stack_addr);

/**
 * Free user stack memory for one task
 * @param stack_addr stack address which will be freed
 */
static void free_user_stack(uint64_t stack_addr);

/**
 * Set Process control block for one task
 * @param pid process id
 * @param pcb pointer to destination pcb
 * @param task_info pointer to task info
 */
void set_pcb(pid_t, pcb_t *, task_info_t *);

void do_process_show();
pid_t do_getpid();
uint64_t get_cpu_id();

void proc_get_lock(mutex_lock_t * lock);
void proc_lose_lock(mutex_lock_t * lock);

#endif