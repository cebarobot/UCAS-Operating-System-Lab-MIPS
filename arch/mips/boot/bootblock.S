#define SECTOR_SZ       0x200
#define BOOT_OFS        0x000
#define KERNEL_OFS      0x200
#define OS_SZ_OFS       0x1fe

#define KERNEL_ADDR     0xffffffffa0800000
#define READ_SD_CARD    0xffffffff8f0d5e10
#define PRINTSTR        0xffffffff8f0d5534

.text
.global main

main:
# copy bootblock to new place (This part is position-independent)
    dla     $a0, main               # dest:     new address of bootblock
    dli     $a1, BOOT_OFS           # offset:   bootblock offset in sd card
    dli     $a2, SECTOR_SZ          # size:     one sector
    dli     $t0, READ_SD_CARD       # read_sd_card(dest, offset, size)
    jalr    $t0

    j       go                      # jump to new bootblock

go:
# call BIOS print string "It's bootblock!"
    dla		$a0, msg                # str:      msg
    dli     $t0, PRINTSTR           # printstr(msg)
    jalr	$t0

# call BIOS read kernel in SD card
    dli     $a0, KERNEL_ADDR        # dest:     start address of kernel
    dli     $a1, KERNEL_OFS         # offset:   kernel offset in sd card
    dla     $a2, main + OS_SZ_OFS   # size:     *(main + os size offset) * 0x200
    lh      $a2, ($a2)
    dmul    $a2, $a2, 0x200
    dli     $t0, READ_SD_CARD       # read_sd_card(dest, offset, size)
    jalr    $t0

# jump to kernel
    dli     $t0, KERNEL_ADDR        # jump to kernel
    jr      $t0

# stop here
stop:
    j       stop

.data

msg: .asciz "It's a bootloader...\r\n"