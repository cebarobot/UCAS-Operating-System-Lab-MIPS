#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"
#include "stdio.h"
#include "syscall.h"

/* exception handler */
uint64_t exception_handler[32];

/* used to init PCB */
uint32_t initial_cp0_status = 0x10008003;

// extern void do_shell();

uint32_t time_before = 0;
uint32_t time_after = 0;
int64_t used_time = 0;

static void irq_timer()
{
    screen_reflush();

    /* increase global time counter */
    update_time_elapsed();

    /* reset timer register */
    reset_timer(TIMER_INTERVAL);

    current_running->last_run = get_timer();

    /* sched.c to do scheduler */
    time_before = get_cp0_count();
    do_scheduler();
    time_after = get_cp0_count();
    used_time = time_after - time_before;
    // average = (average + time_after - time_before) / 2;
    // vt100_move_cursor(1, 31);
    // printk("> [DO SCHEDULER] used time: %d", time_after - time_before);
    // vt100_move_cursor(1, 32);
    // printk("> [DO SCHEDULER] average used time: %d", average);
}

// ! temp
void other_exception_handler(regs_context_t * regs, uint32_t status, uint32_t cause);

/**
 * Determine the kind of interrupt and call their handler
 * ! This function is strong related with architecture
 */
void interrupt_helper(regs_context_t * regs, uint32_t status, uint32_t cause)
{
    // screen_reflush();
    // screen_move_cursor(1, 35);
    // kprintf("> [INTERRUPT] Got an interrupt at 0x%08x", regs->epc);
    // screen_move_cursor(1, 36);
    // kprintf("> [INTERRUPT] current_running is %d [%s]", current_running->pid, current_running->name);

    exccode_t exccode = (cause & CAUSE_EXCCODE) >> 2;
    if (exccode == SYS)                                 // syscall
    {
        // screen_move_cursor(1, 37);
        // kprintf("> [SYSCALL] Syscall of  %d            ", regs->regs[2]);
        // screen_reflush();
        regs->regs[2] = system_call_helper(regs->regs[2], regs->regs[4], regs->regs[5], regs->regs[6]);
        regs->epc += 4;
    }
    else if (exccode == INT && (cause & CAUSE_IP7))     // time interrupt
    {
        // screen_move_cursor(1, 37);
        // kprintf("> [INTERRUPT] Time interrupt at %d    ", time_elapsed);
        // screen_reflush();
        irq_timer();
    }
    else
    {
        // screen_move_cursor(1, 37);
        printk("> [OTHER] exccode: %d\n\r", exccode);
        // screen_reflush();
        other_exception_handler(regs, status, cause);
    }
}

void other_exception_handler(regs_context_t * regs, uint32_t status, uint32_t cause)
{
    char* reg_name[] = 
    {
        "zero ", " at  ", " v0  ", " v1  ", " a0  ", " a1  ", " a2  ", " a3  ",
        " a4  ", " a5  ", " a6  ", " a7  ", " t0  ", " t1  ", " t2  ", " t3  ",
        " s0  ", " s1  ", " s2  ", " s3  ", " s4  ", " s5  ", " s6  ", " s7  ",
        " t8  ", " t9  ", " k0  ", " k1  ", " gp  ", " sp  ", "fp/s8", " ra  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %08x ", reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("cp0_status: 0x%08x badvaddr: 0x%08x cp0_cause: 0x%08x\n\r",
           regs->cp0_status, regs->badvaddr, regs->cp0_cause);
    printk("epc: 0x%08x\n\r", regs->epc);
    while(1);
    // assert(0);
}