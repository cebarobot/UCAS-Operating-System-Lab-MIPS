/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                            Queue Algorithm
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

#ifndef INCLUDE_LIST_H_
#define INCLUDE_LIST_H_

#include "type.h"

typedef struct queue
{
    void *head;
    void *tail;
} queue_t;

/**
 * Initialize a queue
 * @param queue Pointer to the queue
 */
void queue_init(queue_t *queue);

/**
 * Check whether a queue is empty
 * @param queue Pointer to the queue
 * @return 0 for not empty, not 0 for empty
 */
int queue_is_empty(queue_t *queue);

/**
 * Push an item into the queue
 * @param queue Pointer to the queue
 * @param item Pointer to the item
 */
void queue_push(queue_t *queue, void *item);

/**
 * Pop an item from the queue, return a pointer to the item
 * @param queue Pointer to the queue
 * @return Pointer to the popped item.
 */
void *queue_dequeue(queue_t *queue);

/**
 * remove this item and return next item
 * @param queue Pointer to the queue
 * @param item Pointer to the item
 */
void *queue_remove(queue_t *queue, void *item);

#endif