#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"
#include "string.h"
#include "regs.h"
#include "irq.h"
#include "mm.h"

pcb_t pcb_list[NUM_MAX_TASK] = 
{
    [0] = {
        .kernel_sp = STACK_TOP,
        .user_sp = STACK_TOP,
        .pid = 0,
        .name = "system_init"
    }
};

/* current running task PCB */
pcb_t *current_running = pcb_list;

/* global process id */
pid_t process_id = 1;

char task_status_name[][10] = 
{
    "NONE",
    "SLEEPING",
    "BLOCKED",
    "RUNNING",
    "READY",
    "EXITED",
};

/* kernel stack ^_^ */
// #define NUM_KERNEL_STACK 20

static uint64_t kernel_stack_top = STACK_TOP;
static int kernel_stack_used[NUM_MAX_TASK];
static int kernel_stack_count = 0;

static uint64_t user_stack_top = USER_STACK_TOP;
static int user_stack_used[NUM_MAX_TASK];
static int user_stack_count = 0;

// Initialize stack and heap space
void init_stack()
{
    kernel_stack_count = 1;
    kernel_stack_used[0] = 1;
    user_stack_count = 0;
}

// Allocate kernel stack memory for one task
uint64_t new_kernel_stack()
{
    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        if (!kernel_stack_used[i])
        {
            kernel_stack_used[i] = TRUE;
            return kernel_stack_top - i * STACK_SIZE;
        }
    }
    return NULL;
}

// Allocate user stack memory for one task
uint64_t new_user_stack()
{
    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        if (!user_stack_used[i])
        {
            user_stack_used[i] = TRUE;
            return user_stack_top - i * STACK_SIZE;
        }
    }
    return NULL;
}

// Free kernel stack memory for one task
static void free_kernel_stack(uint64_t stack_addr)
{
    int stack_id = (kernel_stack_top - stack_addr) / STACK_SIZE;
    kernel_stack_used[stack_id] = 0;
}

// Free user stack memory for one task
static void free_user_stack(uint64_t stack_addr)
{
    int stack_id = (user_stack_top - stack_addr) / STACK_SIZE;
    user_stack_used[stack_id] = 0;
}

static pcb_t * alloc_pcb() 
{
    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        if (pcb_list[i].status == TASK_NONE) {
            return &pcb_list[i];
        }
    }
    return NULL;
}

static void free_pcb(pcb_t * pcb)
{
    // memset(pcb, 0, sizeof(pcb_t));
    pcb->status = TASK_NONE;
    pcb->pid = 0;
}

static pid_t alloc_pid()
{
    // ! warning: if the program run too many process, then it will boom.
    return process_id ++;
}

static void free_pid(pid_t pid)
{
    // ! warning: if the program run too many process, then it will boom.
    // do nothing
}

// Set Process control block for one task
// ! This part is strong related with architecture
void set_pcb(pcb_t *pcb, pid_t pid, task_info_t *task_info, 
    reg_t kernel_stack, reg_t user_stack, reg_t arg0, reg_t arg1)
{
    // basic info
    pcb->pid = pid;
    pcb->type = task_info->type;
    pcb->priority = task_info->priority;
    memcpy(pcb->name, task_info->name, TASK_NAME_LEN);

    // initialize queue
    pcb->prev = NULL;
    pcb->next = NULL;

    pcb->status = TASK_READY;
    queue_push(&ready_queue, pcb);
    pcb->in_queue = &ready_queue;

    // initialize stack
    pcb->kernel_sp = kernel_stack;
    pcb->user_sp = user_stack;

    // ! This part is strong related with architecture
    // initialize user_context
    pcb->kernel_sp -= sizeof(regs_context_t);
    regs_context_t * user_context = (void *) pcb->kernel_sp;
    memset(user_context, 0, sizeof(regs_context_t));
    user_context->regs[31] = 0;
    user_context->regs[4] = arg0;
    user_context->regs[5] = arg1;
    user_context->cp0_status = initial_cp0_status;
    user_context->epc = task_info->entry_point;

    // initialize kernel_context
    pcb->kernel_sp -= sizeof(regs_context_t);
    regs_context_t * kernel_context = (void *) pcb->kernel_sp;
    memset(kernel_context, 0, sizeof(regs_context_t));
    kernel_context->regs[31] = (reg_t) exception_return;
    kernel_context->cp0_status = initial_cp0_status;

    // initialize locks
    memset(pcb->locks_got, 0, sizeof(pcb->locks_got));

    // initialize cursor
    pcb->cursor_x = 0;
    pcb->cursor_y = 0;
}

void free_proc(pcb_t * pcb)
{
    pid_t pid = pcb->pid;
    // free stack
    free_kernel_stack(pcb->kernel_sp);
    free_user_stack(pcb->user_sp);

    // release locks
    for (int i = 0; i < MAX_LOCK; i++)
    {
        if (pcb->locks_got[i])
        {
            do_mutex_lock_release(pcb->locks_got[i]);
            pcb->locks_got[i] = NULL;
        }
    }

    // awake waitpid proc
    if (!queue_is_empty(&waitpid_queue)) 
    {
        pcb_t * item = waitpid_queue.head;
        pcb_t * next_item;
        while (item != NULL) {
            if (item->waitpid == pid)
            {
                next_item = queue_remove(&waitpid_queue, item);
                item->status = TASK_READY;
                queue_push(&ready_queue, item);
                item->in_queue = &ready_queue;
            }
            else
            {
                next_item = item->next;
            }
            item = next_item;
        }
    }

    // free pcb & pid
    free_pcb(pcb);
    free_pid(pid);
}

/* ready queue to run */
queue_t ready_queue;

// sleep queue 
queue_t sleep_queue;

// waitpid queue
queue_t waitpid_queue;

// TODO: need to optimize
static void check_sleeping()
{
    uint32_t current_time = get_timer();
    if (!queue_is_empty(&sleep_queue)) 
    {
        pcb_t * item = sleep_queue.head;
        pcb_t * next_item;
        while (item != NULL) {
            if (item->sleep_until < current_time)
            {
                next_item = queue_remove(&sleep_queue, item);
                item->status = TASK_READY;
                queue_push(&ready_queue, item);
                item->in_queue = &ready_queue;
            }
            else
            {
                next_item = item->next;
            }
            item = next_item;
        }
    }
}

void scheduler(void)
{
    check_sleeping();

    // handle current running task
    if (current_running->pid && current_running->status == TASK_RUNNING)
    {
        current_running->status = TASK_READY;
        queue_push(&ready_queue, current_running);
        current_running->in_queue = &ready_queue;
    } else if (current_running->status == TASK_EXITED)
    {
        free_proc(current_running);
    }

#ifdef PRIORITY_SCHED
    pcb_t * next_running;
    uint64_t max_priority = 0;
    uint64_t cur_time = get_timer();
    
    for (pcb_t * item = ready_queue.head; item; item = item->next)
    {
        int act_priority = item->priority + cur_time - item->last_run;
        if (act_priority > max_priority) {
            max_priority = act_priority;
            next_running = item;
        }
    }

    // switch
    queue_remove(&ready_queue, next_running);
    current_running = next_running;
    current_running->status = TASK_RUNNING;
    current_running->in_queue = NULL;
    
#else
    current_running = queue_dequeue(&ready_queue);
    current_running->status = TASK_RUNNING;
    current_running->in_queue = NULL;
#endif
}

// TODO: need to optimize
void do_sleep(uint32_t sleep_time)
{
    current_running->status = TASK_SLEEPING;
    current_running->sleep_until = get_timer() + sleep_time;
    queue_push(&sleep_queue, current_running);
    current_running->in_queue = &sleep_queue;

    do_scheduler();
}

// ! unfinished
void do_exit(void)
{
    current_running->status = TASK_EXITED;
    free_proc(current_running);

    do_scheduler();
}

int do_kill(pid_t pid)
{
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb_list[i].pid == pid)
        {
            if (pcb_list[i].status == TASK_RUNNING)
            {
                pcb_list[i].status = TASK_EXITED;
            }
            else
            {
                pcb_list[i].status = TASK_EXITED;
                queue_remove(pcb_list[i].in_queue, &pcb_list[i]);
                free_proc(&pcb_list[i]);
            }
            return 0;
        }
    }
    return -1;
}

// Block current running task into the specific block queue
void do_block(queue_t *queue)
{
    // push task into block queue
    current_running->status = TASK_BLOCKED;
    current_running->in_queue = queue;
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
    item->in_queue = &ready_queue;
    queue_push(&ready_queue, item);
}

// Unblock all tasks in the specific block queue
void do_unblock_all(queue_t *queue)
{
    while (!queue_is_empty(queue)) {
        do_unblock_one(queue);
    }
}

pid_t do_spawn(task_info_t *task, int argc, char** argv)
{
    pcb_t * pcb;
    if (!(pcb = alloc_pcb())) {
        return -1;
    }
    pid_t pid = alloc_pid();

    uint64_t kernel_stack = new_kernel_stack();
    uint64_t user_stack = new_user_stack();
    int arg0 = 0;
    int arg1 = 0;

    if (argc > 0) {
        user_stack -= sizeof(char *) * 16;
        char** new_argv = (void*)user_stack;
        do_TLB_Refill((uint64_t)new_argv);

        for (int i = 0; i < argc; i++) {
            user_stack -= 64;
            new_argv[i] = (void*)user_stack;
            strcpy(new_argv[i], argv[i]);
        }
        arg0 = argc;
        arg1 = (uint64_t)new_argv;
    }
    
    set_pcb(pcb, pid, task, kernel_stack, user_stack, arg0, arg1);

    return pid;
}

int do_waitpid(pid_t pid)
{
    int has_pid = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb_list[i].pid == pid)
        {
            has_pid = 1;
            break;
        }
    }
    if (has_pid)
    {
        current_running->waitpid = pid;
        current_running->status = TASK_BLOCKED;
        queue_push(&waitpid_queue, current_running);
        current_running->in_queue = &waitpid_queue;

        do_scheduler();
    }
}

// process show
void do_process_show()
{
    kprintf("PID, NAME, STATUS\n");
    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        if (pcb_list[i].pid)
        {
            kprintf("%d, %s, ", pcb_list[i].pid, pcb_list[i].name);
            kprintf("%s\n", task_status_name[pcb_list[i].status]);
        }
    }
}

pid_t do_getpid()
{
    return current_running->pid;
}

inline void save_cursor()
{
    current_running->cursor_x = screen_cursor_x;
    current_running->cursor_y = screen_cursor_y;
}

inline void restore_cursor()
{
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
}

inline void proc_get_lock(mutex_lock_t * lock)
{
    for (int i = 0; i < MAX_LOCK; i++)
    {
        if (current_running->locks_got[i] == NULL)
        {
            current_running->locks_got[i] = lock;
            return;
        }
    }
}

inline void proc_lose_lock(mutex_lock_t * lock)
{
    for (int i = 0; i < MAX_LOCK; i++)
    {
        if (current_running->locks_got[i] == lock)
        {
            current_running->locks_got[i] = NULL;
            return;
        }
    }
}