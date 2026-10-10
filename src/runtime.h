#pragma once

#include <cstdio>
#include <cstring>
#include <memory>

#include "fault.h"

constexpr uint32_t OUT_PORT = 0x10000000;
constexpr uint32_t MEM_WORDS = 16384;
constexpr char* DEBUG_FILE = "debug/wave.vcd";

Result run(uint32_t* image, uint64_t max_cycles, const Fault* fault = nullptr) {

    uint32_t ram[MEM_WORDS];
    bzero(ram, sizeof(ram));
    memcpy(ram, image, sizeof(ram));

    const std::unique_ptr<VerilatedContext> ctx{ new VerilatedContext };
    ctx->traceEverOn(true);

    const std::unique_ptr<Vpicorv32> top{
        new Vpicorv32{ctx.get()}
    };

    top->clk = 0;
    top->resetn = 0;
    top->mem_ready = 1;
    top->irq = 0;

    bool got_out = false;
    bool bus_error = false;

    uint32_t out = 0;
    uint64_t cycle = 0;
    uint64_t sim_time = 0;

    VerilatedVcdC tfp;

    top->trace(&tfp, 99);
    tfp.open(DEBUG_FILE);

    while (cycle < max_cycles && !top->trap) {

        top->resetn = (cycle >= 10);

        top->clk = 0;
        top->eval();

        tfp.dump(sim_time++);

        if (top->mem_valid) {
            const uint32_t addr = top->mem_addr;

            if (top->mem_wstrb && addr == OUT_PORT) {

                got_out = true;
                out = top->mem_wdata;

            } else if ((addr >> 2) < MEM_WORDS) {

                const uint32_t idx = addr >> 2;

                if (top->mem_wstrb) {
                    // Memory write
                    for (int b = 0; b < 4; ++b) {
                        if (top->mem_wstrb & (1u << b)) {

                            const uint32_t mask =
                                0xffu << (8 * b);

                            ram[idx] &= ~mask;

                            ram[idx] |=
                                ((top->mem_wdata >> (8 * b)) & 0xffu)
                                << (8 * b);
                        }
                    }
                } else {
                    // Memory read
                    top->mem_rdata = ram[idx];
                }
            } else {

                fprintf(stderr,
                        "bus error at 0x%08x (cycle %llu)\n",
                        addr,
                        static_cast<unsigned long long>(cycle));

                bus_error = true;
                break;
            }
        }

        top->eval();

        if (fault && cycle == fault->cycle)
            apply_fault(*top, *fault);

        top->clk = 1;
        top->eval();

        tfp.dump(sim_time++);

        ++cycle;
    }

    tfp.close();

    Result result{
        .ending = Ending::TIMEOUT,
        .got_out = got_out,
        .out = out,
        .cycles = cycle
    };

    if (bus_error) result.ending = Ending::BUS_ERROR;
    else if (top->trap) result.ending = Ending::TRAP;

    return result;
}