#pragma once

#include <cstdio>
#include <cstring>
#include <memory>
#include "fault.h"

constexpr uint32_t OUT_PORT = 0x10000000;    // program result (any write is recorded)
constexpr uint32_t DETECT_PORT = 0x10000004; // self-check report port
// Detection only counts when the program writes this exact magic value. A bare
// "any write" rule is wrong: a fault in the address register of the normal
// result store can land a stray store on DETECT_PORT and look like a detection.
constexpr uint32_t DETECT_MAGIC = 0xDEADC0DE;
constexpr uint32_t MEM_WORDS = 16384;
inline const char* debugFile = "debug/wave.vcd"; // override with --vcd=

Result run(uint32_t* image, uint64_t max_cycles, const Fault* fault = nullptr, bool enable_trace = false) {
    uint32_t ram[MEM_WORDS];
    bzero(ram, sizeof(ram));
    memcpy(ram, image, sizeof(ram));

    const std::unique_ptr<VerilatedContext> ctx{ new VerilatedContext };
    
    VerilatedVcdC tfp;
    if (enable_trace) {
        ctx->traceEverOn(true);
    }

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
    uint32_t out_count = 0;
    uint64_t out_hash = 0xcbf29ce484222325ULL; // FNV-1a offset basis
    bool detected = false;
    uint64_t cycle = 0;
    uint64_t sim_time = 0;

    if (enable_trace) {
        top->trace(&tfp, 99);
        tfp.open(debugFile);
    }

    while (cycle < max_cycles && !top->trap) {
        top->resetn = (cycle >= 10);
        top->clk = 0;
        top->eval();

        if (enable_trace) tfp.dump(sim_time++);

        if (top->mem_valid) {
            const uint32_t addr = top->mem_addr;

            if (top->mem_wstrb && addr == OUT_PORT) {
                got_out = true;
                out = top->mem_wdata;
                ++out_count;
                out_hash = (out_hash ^ out) * 0x100000001b3ULL;
            } else if (top->mem_wstrb && addr == DETECT_PORT) {
                if (top->mem_wdata == DETECT_MAGIC) detected = true;
            } else if ((addr >> 2) < MEM_WORDS) {
                const uint32_t idx = addr >> 2;

                if (top->mem_wstrb) {
                    for (int b = 0; b < 4; ++b) {
                        if (top->mem_wstrb & (1u << b)) {
                            const uint32_t mask = 0xffu << (8 * b);
                            ram[idx] &= ~mask;
                            ram[idx] |= ((top->mem_wdata >> (8 * b)) & 0xffu) << (8 * b);
                        }
                    }
                } else {
                    top->mem_rdata = ram[idx];
                }
            } else {
                bus_error = true;
                break;
            }
        }

        top->eval();

        if (fault && cycle == fault->cycle)
            apply_fault(*top, *fault);

        top->clk = 1;
        top->eval();

        if (enable_trace) tfp.dump(sim_time++);
        ++cycle;
    }

    if (enable_trace) {
        tfp.close();
    }

    Result result{
        .ending = Ending::TIMEOUT,
        .got_out = got_out,
        .out = out,
        .cycles = cycle,
        .out_count = out_count,
        .out_hash = out_hash,
        .detected = detected
    };

    if (bus_error) result.ending = Ending::BUS_ERROR;
    else if (top->trap) result.ending = Ending::TRAP;

    return result;
}