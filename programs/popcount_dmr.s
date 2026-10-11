# regs: 1 5 6 8 9 10 28 29
# popcount_dmr: same workload as popcount.s, hardened with time-redundant
# execution (DMR). The whole computation runs twice, the two totals are
# compared, and a mismatch is reported by storing the magic value 0xDEADC0DE
# to DETECT_PORT (0x10000004) instead of writing a result to OUT_PORT.
# Expected output: 15 (and no detection in the golden run)
#
# What it cannot cover: a fault that lands after the compare (in s0 or t3
# before the final sw) still ends up as an SDC.
    .section .text
    .globl _start
_start:
    # 1 -> s0
    li   s0, 0
    li   a0, 0x0B6D
    jal  ra, popcount
    add  s0, s0, a0
    li   a0, 0x0D2B
    jal  ra, popcount
    add  s0, s0, a0

    # 2 -> s1
    li   s1, 0
    li   a0, 0x0B6D
    jal  ra, popcount
    add  s1, s1, a0
    li   a0, 0x0D2B
    jal  ra, popcount
    add  s1, s1, a0

    bne  s0, s1, mismatch

    li   t3, 0x10000000
    sw   s0, 0(t3)
    ebreak

mismatch:
    li   t3, 0x10000004     # DETECT_PORT
    li   t4, 0xDEADC0DE     # DETECT_MAGIC
    sw   t4, 0(t3)
    ebreak

# popcount(a0) -> a0
popcount:
    li   t0, 0
pc_loop:
    beqz a0, pc_done
    andi t1, a0, 1
    add  t0, t0, t1
    srli a0, a0, 1
    j    pc_loop
pc_done:
    mv   a0, t0
    ret
