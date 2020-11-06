#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"
#include "string.h"
#include "regs.h"

pcb_t pcb[NUM_MAX_TASK] = 
{
    [0] = {
        .kernel_sp = STACK_TOP,
        .pid = 0,
        .name = "system_init"
    }
};

/* current running task PCB */
pcb_t *current_running = pcb;

/* global process id */
pid_t process_id = 1;

/* kernel stack ^_^ */
// #define NUM_KERNEL_STACK 20

static uint64_t kernel_stack_top = STACK_TOP;
// static uint64_t kernel_stack[NUM_MAX_TASK];
static int kernel_stack_count = 0;

static uint64_t user_stack_top = USER_STACK_TOP;
// static uint64_t user_stack[NUM_MAX_TASK];
static int user_stack_count = 0;

// Initialize stack and heap space
void init_stack()
{
    kernel_stack_count = 1;
    kernel_stack_top -= STACK_SIZE;
    user_stack_count = 0;
}

// Allocate kernel stack memory for one task
uint64_t new_kernel_stack()
{
    kernel_stack_count += 1;
    kernel_stack_top -= STACK_SIZE;

    return kernel_stack_top;
}

// Allocate user stack memory for one task
uint64_t new_user_stack()
{
    user_stack_count += 1;
    user_stack_top -= STACK_SIZE;

    return user_stack_top;
}

// Free kernel stack memory for one task
static void free_kernel_stack(uint64_t stack_addr)
{
}

// Free user stack memory for one task
static void free_user_stack(uint64_t stack_addr)
{
}

// Set Process control block for one task
// ! This part is strong related with architecture
void set_pcb(pid_t pid, pcb_t *pcb, task_info_t *task_info)
{
    // basic info
    pcb->pid = pid;
    pcb->type = task_info->type;
    pcb->status = TASK_READY;
    pcb->priority = 0;
    memcpy(pcb->name, task_info->name, TASK_NAME_LEN);

    // initialize queue
    pcb->prev = NULL;
    pcb->next = NULL;
    queue_push(&ready_queue, pcb);
    pcb->in_queue = &ready_queue;

    // initialize stack
    pcb->kernel_sp = new_kernel_stack();
    pcb->user_sp = new_user_stack();

    // ! This part is strong related with architecture
    // // initialize user_context
    // pcb->kernel_sp -= sizeof(regs_context_t);
    // regs_context_t * user_context = (void *) pcb->kernel_sp;
    // memset(user_context, 0, sizeof(regs_context_t));
    // user_context->regs[31] = 0;
    // user_context->epc = task_info->entry_point;

    // initialize kernel_context
    pcb->kernel_sp -= sizeof(regs_context_t);
    regs_context_t * kernel_context = (void *) pcb->kernel_sp;
    memset(kernel_context, 0, sizeof(regs_context_t));
    kernel_context->regs[31] = task_info->entry_point;

    // initialize cursor
    pcb->cursor_x = 0;
    pcb->cursor_y = 0;
}

/* ready queue to run */
queue_t ready_queue;

/* block queue to wait */
queue_t block_queue;

static void check_sleeping()
{
}

void scheduler(void)
{
    // handle current running task
    if (current_running && current_running->status == TASK_RUNNING)
    {
        queue_push(&ready_queue, current_running);
        current_running->status = TASK_READY;
    }

    // switch
    current_running = queue_dequeue(&ready_queue);
    current_running->status = TASK_RUNNING;
}

void do_sleep(uint32_t sleep_time)
{
}

void do_exit(void)
{
}

// Block current running task into the specific block queue
void do_block(queue_t *queue)
{
    // push task into block queue
    current_running->status = TASK_BLOCKED;
    queue_push(queue, current_running);

    // switch & schedule
    do_scheduler();
}

// Unblock one task in the specific block queue
void do_unblock_one(queue_t *queue)
{
    // pop task from block queue
    pcb_t * item = queue_dequeue(queue);

    // push task into ready queue
    item->status = TASK_READY;
    queue_push(&ready_queue, item);
}

// Unblock all tasks in the specific block queue
void do_unblock_all(queue_t *queue)
{
    while (!queue_is_empty(queue)) {
        do_unblock_one(queue);
    }
}

int do_spawn(task_info_t *task)
{
}

int do_kill(pid_t pid)
{
}

int do_waitpid(pid_t pid)
{
}

// process show
void do_process_show()
{
}

pid_t do_getpid()
{
    return current_running->pid;
}