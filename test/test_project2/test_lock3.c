#include "lock.h"
#include "stdio.h"
#include "syscall.h"
#include "test.h"

#define LOCK_TIME 20
#define WAITE_TIME 10

static char blank[] = {"                                             "};

const int mutex_key = 1345;

void lock_task1(void)
{
        int print_location = 5;
        const int lock_id = binsem_get(mutex_key);
        while (1)
        {
                int i;

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                for (i = 0; i < WAITE_TIME; i++)
                {
                        sys_move_cursor(1, print_location);
                        printf("> [TASK] Applying for a lock %d.\n", i);
                }
                binsem_op(lock_id, BINSEM_OP_LOCK);

                for (i = 0; i < LOCK_TIME; i++)
                {
                        sys_move_cursor(1, print_location);
                        printf("> [TASK] Has acquired lock and running.(%d)\n", i);
                }

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                sys_move_cursor(1, print_location);
                printf("> [TASK] Has acquired lock and exited.\n");

                binsem_op(lock_id, BINSEM_OP_UNLOCK);
        }
}

void lock_task2(void)
{
        int print_location = 6;
        const int lock_id = binsem_get(mutex_key);
        while (1)
        {
                int i;

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                for (i = 0; i < WAITE_TIME; i++)
                {
                        sys_move_cursor(1, print_location);
                        printf("> [TASK] Applying for a lock. %d\n", i);
                }

                binsem_op(lock_id, BINSEM_OP_LOCK);

                for (i = 0; i < LOCK_TIME; i++)
                {
                        sys_move_cursor(1, print_location);
                        printf("> [TASK] Has acquired lock and running.(%d)\n", i);
                }

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                sys_move_cursor(1, print_location);
                printf("> [TASK] Has acquired lock and exited.\n");

                binsem_op(lock_id, BINSEM_OP_UNLOCK);
        }
}
