# regs: 1 5 6 8 10 28
# popcount: total number of set bits in two constants, via a subroutine.
# 0x0B6D has 8 set bits, 0x0D2B has 7 -> expected output: 15
# Stresses shifts/AND (logical masking), a data-dependent loop exit, and the
# call/return path (ra).
    .section .text
    .globl _start
_start:
    li   s0, 0              # total = 0

    li   a0, 0x0B6D
    jal  ra, popcount
    add  s0, s0, a0

    li   a0, 0x0D2B
    jal  ra, popcount
    add  s0, s0, a0

    li   t3, 0x10000000
    sw   s0, 0(t3)
    ebreak

# popcount(a0) -> a0.  Clobbers t0, t1.
popcount:
    li   t0, 0              # count = 0
pc_loop:
    beqz a0, pc_done
    andi t1, a0, 1
    add  t0, t0, t1
    srli a0, a0, 1
    j    pc_loop
pc_done:
    mv   a0, t0
    ret
