#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"
#include "stdio.h"

/* exception handler */
uint64_t exception_handler[32];

/* used to init PCB */
uint32_t initial_cp0_status = 0x10008003;

// extern void do_shell();

static void irq_timer()
{
    // screen_reflush();

    /* increase global time counter */
    time_elapsed = get_timer();

    /* reset timer register */
    reset_timer(TIMER_INTERVAL);

    /* sched.c to do scheduler */
    do_scheduler();
}

// ! temp
void other_exception_handler(regs_context_t * regs, uint32_t status, uint32_t cause);

/**
 * Determine the kind of interrupt and call their handler
 * ! This function is strong related with architecture
 */
void interrupt_helper(regs_context_t * regs, uint32_t status, uint32_t cause)
{
    // vt100_move_cursor(1, 40);
    // printk("%s", COLOR_YELLOW);
    // printk("> [INTERRUPT] Got an interrupt at 0x%08x\n\r", regs->epc);
    // printk("> [INTERRUPT] current_running is %d [%s]\n\r", current_running->pid, current_running->name);

    exccode_t exccode = (cause & CAUSE_EXCCODE) >> 2;
    if (exccode == SYS)                                 // syscall
    {
        system_call_helper();
    }
    else if (exccode == INT && (cause & CAUSE_IP7))     // time interrupt
    {
        // printk("> [INTERRUPT] Time interrupt at %d\n\r", time_elapsed);
        irq_timer();
    }
    else
    {
        other_exception_handler(regs, status, cause);
    }
    // printk("%s", COLOR_RESET);
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
            printk("%s : %016x ", reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("cp0_status: 0x%8x badvaddr: 0x%8x scause: %8x\n\r",
           regs->cp0_status, regs->badvaddr, regs->cp0_cause);
    printk("epc: 0x%lx\n\r", regs->epc);
    int * whh = (void *) 0x12345678;
    (*whh) = 100;
    // assert(0);
}