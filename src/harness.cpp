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

#include "Vpicorv32.h"
#include "Vpicorv32___024root.h"
#include "verilated_vcd_c.h"
#include "verilated.h"
#include "runtime.h"

uint32_t image[MEM_WORDS];

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    if (argc < 2) {
        fprintf(stderr, "usage: %s program.bin\n", argv[0]);
        return 1;
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        perror("open");
        return 1;
    }

    const size_t bytes_read =
        fread(image, 1, sizeof(image), f);

    if (ferror(f)) {
        perror("read");
        fclose(f);
        return 1;
    }

    fclose(f);

    const uint64_t max_cycles = 1000000;

    const Result golden = run(image, max_cycles, nullptr);

    printf("GOLDEN:\n");
    printf("\tOUT:\t0x%08x\n", golden.out);
    printf("\tENDING:\t%s\n", ending_to_string(golden.ending));
    printf("\tCYCLES:\t%llu\n", static_cast<unsigned long long>(golden.cycles));

    Fault fault{
        .tgt = &targets[0],
        .idx = 0,
        .bit = 4,
        .cycle = 20
    };

    const Result tampered = run(image, max_cycles, &fault);
    const Outcome outcome = classify(tampered, golden);

    printf("\nFAULT:\n");
    printf("\tTARGET\t%s[%d]\n", fault.tgt->name, fault.idx);

    printf("\tBIT:\t%d\n", fault.bit);
    printf("\tCYCLE:\t%llu\n", static_cast<unsigned long long>(fault.cycle));

    printf("\nTAMPERED:\n");
    printf("\tOUT:\t0x%08x\n", tampered.out);
    printf("\tENDING:\t%s\n", ending_to_string(tampered.ending));
    printf("\tCYCLES:\t%llu\n", static_cast<unsigned long long>(tampered.cycles));
    printf("\tOUTCOME\t%s\n", outcome_to_string(outcome));

    return 0;
}