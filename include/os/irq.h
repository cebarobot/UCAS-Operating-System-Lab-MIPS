/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                  IRQ
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

#ifndef INCLUDE_INTERRUPT_H_
#define INCLUDE_INTERRUPT_H_

#include "type.h"
#include "sched.h"

/* ERROR code */
typedef enum
{
    /* 14, 16-22, 24-31 is reserver ExcCode */
    INT,       // 0
    MOD,       // 1
    TLBL,      // 2
    TLBS,      // 3
    ADEL,      // 4
    ADES,      // 5
    IBE,       // 6
    DBE,       // 7
    SYS,       // 8
    BP,        // 9
    RI,        // 10
    CPU,       // 11
    OV,        // 12
    TR,        // 13
    FPE = 15,  // 15
    WATCH = 23 // 23
} exccode_t;

#define CAUSE_EXCCODE   0x7c
#define CAUSE_IP0       0x100
#define CAUSE_IP1       0x200
#define CAUSE_IP2       0x400
#define CAUSE_IP3       0x800
#define CAUSE_IP4       0x1000
#define CAUSE_IP5       0x2000
#define CAUSE_IP6       0x4000
#define CAUSE_IP7       0x8000

/* BEV = 0 */
#define BEV0_EBASE 0xffffffff80000000
#define BEV0_OFFSET 0x180

/* BEV = 1 */
#define BEV1_EBASE 0xffffffffbfc00000
#define BEV1_OFFSET 0x180

#define TIMER_INTERVAL 150000

/**
 * Determine the kind of interrupt and call their handler
 * ! This function is strong related with architecture
 * @param regs regs of user context
 * @param status cp0 status
 * @param cause cp0 cause
 */
void interrupt_helper(regs_context_t * regs, uint32_t status, uint32_t cause);

extern void reset_timer(uint32_t);
extern void set_cp0_status(uint32_t);
extern void set_cp0_cause(uint32_t);
extern void set_cp0_compare(uint32_t);
extern uint32_t get_cp0_compare(void);
extern uint32_t get_cp0_cause(void);
extern uint32_t get_cp0_status(void);   

/* exception handler entry */
extern void exception_handler_entry(void);
extern void exception_handler_begin(void);
extern void exception_handler_end(void);
extern void exception_return(void);

extern void handle_int(void);
extern void handle_syscall(void);
extern void handle_other(void);
extern void handle_tlb(void);
extern void TLBexception_handler_begin(void);
extern void TLBexception_handler_end(void);
extern void TLBexception_handler_entry(void);
extern uint32_t initial_cp0_status;
extern uint64_t exception_handler[32];
//extern uint64_t *exception_handler;
#endif