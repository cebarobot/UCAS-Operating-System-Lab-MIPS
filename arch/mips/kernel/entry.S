
#include "asm.h"

# .equ    KERNEL, 0
# .equ    USER,   312
# .equ    NESTED_COUNT, 624


LEAF(asm_start)
    mtc0    $zero, CP0_STATUS
    mtc0    $zero, CP0_WATCHLO
    mtc0    $zero, CP0_WATCHHI

    mfc0    $t0, CP0_CONFIG
    and     $t0, ~0x7
    ori     $t0, 0x2
    mtc0    $t0, CP0_CONFIG

    # Initialize $k0 with pcb[0]
    dla     $k0, pcb
    # Initialize $sp with STACK_TOP
    dli     $sp, 0xffffffffa0f00000
    # Initialize CP0_STATUS

    # TODO: Need to initialize $gp

    jr      $ra
END(asm_start)

.macro SAVE_CONTEXT
    # diable warning of using $at
    .set    noat

    # allocate stack for context 
    addi    $sp, $sp, -(OFFSET_SIZE)

    # saving main processor regs
    # sd      $0, OFFSET_REG0($sp)      # zero
    sd      $1, OFFSET_REG1($sp)
    sd      $2, OFFSET_REG2($sp)
    sd      $3, OFFSET_REG3($sp)
    sd      $4, OFFSET_REG4($sp)
    sd      $5, OFFSET_REG5($sp)
    sd      $6, OFFSET_REG6($sp)
    sd      $7, OFFSET_REG7($sp)
    sd      $8, OFFSET_REG8($sp)
    sd      $9, OFFSET_REG9($sp)
    sd      $10, OFFSET_REG10($sp)
    sd      $11, OFFSET_REG11($sp)
    sd      $12, OFFSET_REG12($sp)
    sd      $13, OFFSET_REG13($sp)
    sd      $14, OFFSET_REG14($sp)
    sd      $15, OFFSET_REG15($sp)
    sd      $16, OFFSET_REG16($sp)
    sd      $17, OFFSET_REG17($sp)
    sd      $18, OFFSET_REG18($sp)
    sd      $19, OFFSET_REG19($sp)
    sd      $20, OFFSET_REG20($sp)
    sd      $21, OFFSET_REG21($sp)
    sd      $22, OFFSET_REG22($sp)
    sd      $23, OFFSET_REG23($sp)
    sd      $24, OFFSET_REG24($sp)
    sd      $25, OFFSET_REG25($sp)
    # sd      $26, OFFSET_REG26($sp) # k0
    # sd      $27, OFFSET_REG27($sp) # k1
    sd      $28, OFFSET_REG28($sp)
    # sd      $29, OFFSET_REG29($sp) # sp
    sd      $30, OFFSET_REG30($sp)
    sd      $31, OFFSET_REG31($sp)

    # saving cp0 processor regs
    mfc0    $k1, CP0_STATUS
    sd      $k1, OFFSET_STATUS($sp)

    mfc0    $k1, CP0_CAUSE
    sd      $k1, OFFSET_CAUSE($sp)

    mfhi    $k1
    sd      $k1, OFFSET_HI($sp)

    mflo    $k1
    sd      $k1, OFFSET_LO($sp)

    mfc0    $k1, CP0_BADVADDR
    sd      $k1, OFFSET_BADVADDR($sp)

    mfc0    $k1, CP0_EPC
    sd      $k1, OFFSET_EPC($sp)

    # enable warning of using $at
    .set    at
.endm

.macro RESTORE_CONTEXT
    # diable warning of using $at
    .set    noat

    # restoring
    # ld      $0, OFFSET_REG0($sp)  # zero
    ld      $1, OFFSET_REG1($sp)
    ld      $2, OFFSET_REG2($sp)
    ld      $3, OFFSET_REG3($sp)
    ld      $4, OFFSET_REG4($sp)
    ld      $5, OFFSET_REG5($sp)
    ld      $6, OFFSET_REG6($sp)
    ld      $7, OFFSET_REG7($sp)
    ld      $8, OFFSET_REG8($sp)
    ld      $9, OFFSET_REG9($sp)
    ld      $10, OFFSET_REG10($sp)
    ld      $11, OFFSET_REG11($sp)
    ld      $12, OFFSET_REG12($sp)
    ld      $13, OFFSET_REG13($sp)
    ld      $14, OFFSET_REG14($sp)
    ld      $15, OFFSET_REG15($sp)
    ld      $16, OFFSET_REG16($sp)
    ld      $17, OFFSET_REG17($sp)
    ld      $18, OFFSET_REG18($sp)
    ld      $19, OFFSET_REG19($sp)
    ld      $20, OFFSET_REG20($sp)
    ld      $21, OFFSET_REG21($sp)
    ld      $22, OFFSET_REG22($sp)
    ld      $23, OFFSET_REG23($sp)
    ld      $24, OFFSET_REG24($sp)
    ld      $25, OFFSET_REG25($sp)
    # ld      $26, OFFSET_REG26     # k0
    # ld      $27, OFFSET_REG27     # k1
    ld      $28, OFFSET_REG28($sp)
    # ld      $29, OFFSET_REG29($sp) # sp
    ld      $30, OFFSET_REG30($sp)
    ld      $31, OFFSET_REG31($sp)

    # restoring cp0 processor regs
    ld      $k1, OFFSET_STATUS($sp)
    mtc0    $k1, CP0_STATUS

    ld      $k1, OFFSET_CAUSE($sp)
    mtc0    $k1, CP0_CAUSE

    ld      $k1, OFFSET_HI($sp)
    mthi    $k1

    ld      $k1, OFFSET_LO($sp)
    mtlo    $k1

    ld      $k1, OFFSET_BADVADDR($sp)
    mtc0    $k1, CP0_BADVADDR

    ld      $k1, OFFSET_EPC($sp)
    mtc0    $k1, CP0_EPC

    # free stack of context
    addi    $sp, $sp, (OFFSET_SIZE)

    # enable warning of using $at
    .set    at
.endm

# function do_scheduler
NESTED(do_scheduler, 0, $ra)
    # save context for old task kernel
    SAVE_CONTEXT
    # save sp for old task kernel
    sd      $sp, OFFSET_KERNEL_SP($k0)

    # call scheduler()
    jal     scheduler
    # nop
    ld      $k0, current_running

    # restore sp for new task kernel
    ld      $sp, OFFSET_KERNEL_SP($k0)
    # restore context for new task kernel
    RESTORE_CONTEXT
    # return to new task kernel
    jr      $ra
END(do_scheduler)

LEAF(reset_timer)
    # mtc0    $zero, CP0_COUNT
    # mtc0    $a0, CP0_COMPARE
    mfc0    $t0, CP0_COUNT
    add     $t0, $t0, $a0
    mtc0    $t0, CP0_COMPARE
    jr      $ra
END(reset_timer)

LEAF(set_cp0_status)
    mtc0    $a0, CP0_STATUS
    jr      $ra
END(set_cp0_status)

LEAF(set_cp0_cause)
    mtc0    $a0, CP0_CAUSE
    jr      $ra
END(set_cp0_cause)

LEAF(set_cp0_compare)
    mtc0    $a0, CP0_COMPARE
    jr      $ra
END(set_cp0_cause)

LEAF(get_cp0_status)
    mfc0    $v0, CP0_STATUS
    jr      $ra
END(get_cp0_status)

LEAF(get_cp0_cause)
    mfc0    $v0, CP0_CAUSE
    jr      $ra
END(get_cp0_cause)

LEAF(get_cp0_count)
    mfc0    $v0, CP0_COUNT
    jr      $ra
END(get_cp0_count)

LEAF(get_cp0_compare)
    mfc0    $v0, CP0_COMPARE
    jr      $ra
END(get_cp0_compare)


.global exception_handler_begin
.global exception_handler_end
.global TLBexception_handler_begin
.global TLBexception_handler_end

NESTED(TLBexception_handler_entry, 0, $sp)
TLBexception_handler_begin:

TLBexception_handler_end:
END(TLBexception_handler_entry)

NESTED(exception_handler_entry, 0, $ra)
exception_handler_begin:
    # switch to kernel stack & save user context
    sd      $sp, OFFSET_USER_SP($k0)
    ld      $sp, OFFSET_KERNEL_SP($k0)
    SAVE_CONTEXT

    # set return address to exception_return
    dla     $ra, exception_return
    # prepare argvs for interrupt_helper
    move    $a0, $sp            # param: regs
    mfc0    $a1, CP0_STATUS     # param: status
    mfc0    $a2, CP0_CAUSE      # param: cause
    # call interrupt_helper
    dla     $k1, interrupt_helper
    jr      $k1

exception_handler_end:
END(exception_handler_entry)

NESTED(exception_return, 0, $ra)
    # retore user context & switch to user stack
    RESTORE_CONTEXT
    sd      $sp, OFFSET_KERNEL_SP($k0)
    ld      $sp, OFFSET_USER_SP($k0)

    # exception return
    eret
END(exception_return)


NESTED(handle_int, 0, $sp)


END(handle_int)

NESTED(handle_syscall, 0, $sp)
# put args in correct place
    move    $a3, $a2
    move    $a2, $a1
    move    $a1, $a0
    move    $a0, $v0

# jump into system_call_helper
    # jal     

# return
    jr      $ra
END(handle_syscall)

NESTED(handle_tlb, 0, $sp)

END(handle_tlb)

NESTED(handle_other, 0, $sp)


END(handle_other)

LEAF(exception_handler_exit)

END(exception_handler_exit)

LEAF(set_cp0_entryhi)

END(set_cp0_entryhi)


LEAF(get_cp0_index)

END(get_cp0_index)

LEAF(set_cp0_index)

END(set_cp0_index)

LEAF(get_cp0_badvaddr)

END(get_cp0_badvaddr)

LEAF(get_cp0_entrylo0)

END(get_cp0_entrylo0)

LEAF(set_cp0_entrylo0)

END(set_cp0_entrylo0)

LEAF(get_cp0_entrylo1)

END(get_cp0_entrylo1)

LEAF(set_cp0_entrylo1)

END(set_cp0_entrylo1)

LEAF(set_cp0_pagemask)

END(set_cp0_pagemask)

LEAF(tlbwr_operation)

END(tlbwr_operation)

LEAF(tlbwi_operation)

END(tlbwi_operation)

LEAF(tlbp_operation)

END(tlbp_operation)



LEAF(get_cp0_config)

END(get_cp0_config)

LEAF(get_cpu_id)

END(get_cpu_id)

