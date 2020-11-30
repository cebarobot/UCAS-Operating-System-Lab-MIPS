#include "barrier.h"
#include "sched.h"

void do_barrier_init(barrier_t *barrier, int goal)
{
    barrier->goal = goal;
    barrier->now = 0;
}

void do_barrier_wait(barrier_t *barrier)
{
    barrier->now += 1;
    if (barrier->now < barrier->goal)
    {
        do_block(&barrier->block_queue);
    } 
    else
    {
        do_unblock_all(&barrier->block_queue);
        barrier->now = 0;
    }
}