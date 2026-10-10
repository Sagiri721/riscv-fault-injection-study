    .section .text
    .globl _start
_start:
    li   t0, 0            # sum = 0
    li   t1, 1            # i = 1
    li   t2, 11           # limit

loop:
    add  t0, t0, t1       # sum += i
    addi t1, t1, 1
    blt  t1, t2, loop
    li   t3, 0x10000000
    sw   t0, 0(t3)
    ebreak                # raise trap