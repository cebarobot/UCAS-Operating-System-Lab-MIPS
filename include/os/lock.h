/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Thread Lock
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

#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_

#include "queue.h"

#define BINSEM_OP_LOCK 0
#define BINSEM_OP_UNLOCK 1
#define NUM_BINSEM 32

typedef enum
{
    UNLOCKED,
    LOCKED,
} lock_status_t;

typedef struct spin_lock
{
    lock_status_t status;
} spin_lock_t;

typedef struct mutex_lock
{
    lock_status_t status;
    queue_t block_queue;

} mutex_lock_t;

extern mutex_lock_t binsem_list[NUM_BINSEM];

/* init lock */
void spin_lock_init(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

/** 
 * Initialize a mutex lock
 * @param lock the lock
 */
void do_mutex_lock_init(mutex_lock_t *lock);

/** 
 * Acquire a mutex lock
 * @param lock the lock
 */
void do_mutex_lock_acquire(mutex_lock_t *lock);

/** 
 * Release a mutex lock
 * @param lock the lock
 */
void do_mutex_lock_release(mutex_lock_t *lock);

/**
 * Binsem Operation
 * @param binsem_id binsem id which can be got by do_binsem_get
 * @param op operation (BINSEM_OP_LOCK, BINSEM_OP_UNLOCK)
 */
void do_binsem_op(uint64_t binsem_id, int op);

/**
 * Binsem Operation
 * @param key same key will get same binsem id
 * @return binsem id
 */
uint64_t do_binsem_get(int key);

#endif
