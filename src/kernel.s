.set noreorder
.section .text

.macro sysc code, label
    .globl \label
    .type \label,@function
    \label:
        addiu      $a0, $zero, \code
        syscall    0
        jr         $ra
        nop
.endm

.macro kern table, code, label
    .globl \label
    .type \label,@function
    \label:
        addiu      $t2, $zero, \table
        jr         $t2
        addiu      $t1, $zero, \code
.endm

sysc 1, EnterCriticalSection
sysc 2, ExitCriticalSection

#kern 0xA0, 0x0D, strtol # the libc one is too big
kern 0xA0, 0x10, atoi   # the libc one is too big
kern 0xA0, 0x24, strstr   # the libc one doesn't work?
kern 0xA0, 0x3E, puts
kern 0xA0, 0x3F, printf
kern 0xB0, 0x04, enable_timer_irq
kern 0xB0, 0x05, disable_timer_irq
kern 0xB0, 0x08, OpenEvent
kern 0xB0, 0x09, CloseEvent
kern 0xB0, 0x0C, EnableEvent
kern 0xB0, 0x0D, DisableEvent

