#include "lock.h"
#include "sched.h"
#include "syscall.h"

mutex_lock_t binsem_list[NUM_BINSEM];

void spin_lock_init(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

void spin_lock_acquire(spin_lock_t *lock)
{
    while (LOCKED == lock->status)
        ;

    lock->status = LOCKED;
}

void spin_lock_release(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

// Initialize a mutex lock
void do_mutex_lock_init(mutex_lock_t *lock)
{
    lock->status = UNLOCKED;
    queue_init(&lock->block_queue);
}

// Acquire a mutex lock
void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    if (lock->status == UNLOCKED)
    {
        lock->status = LOCKED;
        // TODO: need to mark the task have this lock.
    }
    else
    {
        do_block(&lock->block_queue);
    }
}

// Release a mutex lock
void do_mutex_lock_release(mutex_lock_t *lock)
{
    if (queue_is_empty(&lock->block_queue))
    {
        lock->status = UNLOCKED;
    }
    else
    {
        do_unblock_one(&lock->block_queue);
    }
}

void do_binsem_op(uint64_t binsem_id, int op)
{
    if (op == BINSEM_OP_LOCK)
    {
        do_mutex_lock_acquire(&binsem_list[binsem_id]);
    }
    else if (op == BINSEM_OP_UNLOCK)
    {
        do_mutex_lock_release(&binsem_list[binsem_id]);
    }
}

uint64_t do_binsem_get(int key)
{
    return key % NUM_BINSEM;
}