#include "asm.h"

LEAF(invoke_syscall)
    move    $v0, $a0
    move    $a0, $a1
    move    $a1, $a2
    move    $a2, $a3

    syscall
    jr      $ra
END(invoke_syscall)