#include "sched.h"
#include "stdio.h"
#include "syscall.h"
#include "test.h"

extern int64_t used_time;

void cost_task(void)
{
    int total = 0;
    for (int i = 1; ; i++)
    {
        total += used_time;
        sys_move_cursor(1, 28);
        printf("> [DO SCHEDULER] used time: %d     ", used_time);
        sys_move_cursor(1, 29);
        printf("> [DO SCHEDULER] average used time: %d     ", total / i);
    }
}
