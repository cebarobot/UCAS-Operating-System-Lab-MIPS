
#include "asm.h"

.equ    KERNEL, 0
.equ    USER,   312
.equ    NESTED_COUNT, 624

 
LEAF(asm_start)
    mtc0    zero, CP0_STATUS
    mtc0    zero, CP0_WATCHLO
    mtc0    zero, CP0_WATCHHI

    mfc0    t0, CP0_CONFIG
    and     t0, ~0x7
    ori     t0, 0x2
    mtc0    t0, CP0_CONFIG
    
    jr  ra
END(asm_start)

.macro SAVE_CONTEXT offset
    .set    noat
    # compute context address
    ld      k0, current_running
    dadd    k0, k0, \offset

    # saving
    # sd      $0, OFFSET_REG0(k0)      # zero
    sd      $1, OFFSET_REG1(k0)
    sd      $2, OFFSET_REG2(k0)
    sd      $3, OFFSET_REG3(k0)
    sd      $4, OFFSET_REG4(k0)
    sd      $5, OFFSET_REG5(k0)
    sd      $6, OFFSET_REG6(k0)
    sd      $7, OFFSET_REG7(k0)
    sd      $8, OFFSET_REG8(k0)
    sd      $9, OFFSET_REG9(k0)
    sd      $10, OFFSET_REG10(k0)
    sd      $11, OFFSET_REG11(k0)
    sd      $12, OFFSET_REG12(k0)
    sd      $13, OFFSET_REG13(k0)
    sd      $14, OFFSET_REG14(k0)
    sd      $15, OFFSET_REG15(k0)
    sd      $16, OFFSET_REG16(k0)
    sd      $17, OFFSET_REG17(k0)
    sd      $18, OFFSET_REG18(k0)
    sd      $19, OFFSET_REG19(k0)
    sd      $20, OFFSET_REG20(k0)
    sd      $21, OFFSET_REG21(k0)
    sd      $22, OFFSET_REG22(k0)
    sd      $23, OFFSET_REG23(k0)
    sd      $24, OFFSET_REG24(k0)
    sd      $25, OFFSET_REG25(k0)
    # sd      $26, OFFSET_REG26(k0) # k0
    # sd      $27, OFFSET_REG27(k0) # k1
    sd      $28, OFFSET_REG28(k0)
    sd      $29, OFFSET_REG29(k0)
    sd      $30, OFFSET_REG30(k0)
    sd      $31, OFFSET_REG31(k0)
    .set    at
.endm

.macro RESTORE_CONTEXT offset
    .set    noat
    # compute context address
    ld      k0, current_running
    dadd    k0, k0, \offset

    # restoring
    # ld      $0, OFFSET_REG0(k0)  # zero
    ld      $1, OFFSET_REG1(k0)
    ld      $2, OFFSET_REG2(k0)
    ld      $3, OFFSET_REG3(k0)
    ld      $4, OFFSET_REG4(k0)
    ld      $5, OFFSET_REG5(k0)
    ld      $6, OFFSET_REG6(k0)
    ld      $7, OFFSET_REG7(k0)
    ld      $8, OFFSET_REG8(k0)
    ld      $9, OFFSET_REG9(k0)
    ld      $10, OFFSET_REG10(k0)
    ld      $11, OFFSET_REG11(k0)
    ld      $12, OFFSET_REG12(k0)
    ld      $13, OFFSET_REG13(k0)
    ld      $14, OFFSET_REG14(k0)
    ld      $15, OFFSET_REG15(k0)
    ld      $16, OFFSET_REG16(k0)
    ld      $17, OFFSET_REG17(k0)
    ld      $18, OFFSET_REG18(k0)
    ld      $19, OFFSET_REG19(k0)
    ld      $20, OFFSET_REG20(k0)
    ld      $21, OFFSET_REG21(k0)
    ld      $22, OFFSET_REG22(k0)
    ld      $23, OFFSET_REG23(k0)
    ld      $24, OFFSET_REG24(k0)
    ld      $25, OFFSET_REG25(k0)
    # ld      $26, OFFSET_REG26     # k0
    # ld      $27, OFFSET_REG27     # k1
    ld      $28, OFFSET_REG28(k0)
    ld      $29, OFFSET_REG29(k0)
    ld      $30, OFFSET_REG30(k0)
    ld      $31, OFFSET_REG31(k0)
    .set    at
.endm

# function do_scheduler
NESTED(do_scheduler, 0, ra)
    SAVE_CONTEXT(KERNEL)
    jal     scheduler
    RESTORE_CONTEXT(KERNEL)
    jr      ra

END(do_scheduler)

LEAF(reset_timer)
    
END(reset_timer)

LEAF(set_cp0_status)
    
END(set_cp0_status)

LEAF(set_cp0_cause)
   
END(set_cp0_cause)

LEAF(get_cp0_status)
    
END(get_cp0_status)

LEAF(get_cp0_cause)
    
END(get_cp0_cause)

LEAF(get_cp0_count)
	 
END(get_cp0_count)

LEAF(get_cp0_compare)
	 
END(get_cp0_compare)

 
.global exception_handler_begin
.global exception_handler_end
.global TLBexception_handler_begin
.global TLBexception_handler_end

NESTED(TLBexception_handler_entry, 0, sp)
TLBexception_handler_begin:
	 
 
TLBexception_handler_end:
END(TLBexception_handler_entry)

NESTED(exception_handler_entry, 0, sp)   
exception_handler_begin:
    
exception_handler_end:
END(exception_handler_entry)

NESTED(handle_int, 0, sp)
    
    
END(handle_int)

NESTED(handle_syscall, 0, sp)
   
    
END(handle_syscall)

NESTED(handle_tlb, 0, sp)
   
END(handle_tlb)

NESTED(handle_other, 0, sp)
    
   
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

