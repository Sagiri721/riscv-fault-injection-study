# regs: 5 6 7 28 29 30
# memsum: store 1..10 into a scratch array, read it back and add it up.
# Expected output: 55
# Stresses the load/store path and pointer registers: a flipped pointer bit
# either lands in the wrong word (SDC) or outside RAM (BUS_ERROR -> CRASH).
    .section .text
    .globl _start
_start:
    li   t4, 0x1000         # t4 = write pointer (scratch array, clear of the code)
    li   t1, 1              # i = 1
    li   t2, 11             # limit
fill:
    sw   t1, 0(t4)
    addi t4, t4, 4
    addi t1, t1, 1
    blt  t1, t2, fill

    li   t4, 0x1000         # t4 = read pointer
    li   t0, 0              # sum = 0
    li   t1, 10             # words remaining
drain:
    lw   t5, 0(t4)
    add  t0, t0, t5
    addi t4, t4, 4
    addi t1, t1, -1
    bnez t1, drain

    li   t3, 0x10000000
    sw   t0, 0(t3)
    ebreak
