/**
 * Esta harness tem como propósito fornecer o CPU simulado
 * de RISCV com um clock e memoria e cenas
*/

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <functional>
#include <cstring>
#include <string>
#include <vector>

#include "Vpicorv32.h"
#include "Vpicorv32___024root.h"
#include "verilated_vcd_c.h"
#include "verilated.h"

#include "campaign.h"

uint32_t image[MEM_WORDS];

inline static void usage(const char* prog) {
    fprintf(stderr,
        "usage: %s program.bin [results.csv] [--regs=5,6,7] [--stride=N] [--vcd=path]\n"
        "  --regs    cpuregs indices to inject into (default: all of x1..x31)\n"
        "  --stride  inject every N-th cycle instead of every cycle (default: 1)\n"
        "  --vcd     where to write the golden-run waveform (default: debug/wave.vcd)\n",
        prog);
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    const char* binPath = nullptr;
    const char* resultsPath = "debug/fault_results.csv";
    std::vector<int> regs;
    uint64_t stride = 1;
    int positional = 0;

    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--regs=", 7) == 0) {
            std::string list = argv[i] + 7;
            for (char& c : list) if (c == ',') c = ' ';
            const char* p = list.c_str();
            char* end = nullptr;
            for (;;) {
                long v = std::strtol(p, &end, 10);
                if (end == p) break;
                regs.push_back(static_cast<int>(v));
                p = end;
            }
        } else if (std::strncmp(argv[i], "--vcd=", 6) == 0) {
            debugFile = argv[i] + 6;
        } else if (std::strncmp(argv[i], "--stride=", 9) == 0) {
            stride = std::strtoull(argv[i] + 9, nullptr, 10);
        } else if (std::strncmp(argv[i], "--", 2) == 0) {
            usage(argv[0]);
            return 1;
        } else if (positional == 0) {
            binPath = argv[i]; ++positional;
        } else if (positional == 1) {
            resultsPath = argv[i]; ++positional;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (!binPath) {
        usage(argv[0]);
        return 1;
    }

    if (regs.empty()) {
        for (int r = 1; r < 32; ++r) regs.push_back(r); // x0 is hardwired to zero
    }

    FILE* f = fopen(binPath, "rb");
    if (!f) {
        perror("open");
        return 1;
    }

    const size_t bytes_read = fread(image, 1, sizeof(image), f);

    if (ferror(f)) {
        perror("read");
        fclose(f);
        return 1;
    }

    fclose(f);

    if (bytes_read == 0) {
        fprintf(stderr, "program image is empty: %s\n", binPath);
        return 1;
    }

    const uint64_t maxCycles = 1000000;

    return runCampaign(image, maxCycles, regs.data(), static_cast<int>(regs.size()),
                       resultsPath, stride) ? 0 : 1;
}
