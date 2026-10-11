#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "Vpicorv32.h"
#include "Vpicorv32___024root.h"

enum class Ending  { TRAP, BUS_ERROR, TIMEOUT };
enum class Outcome { MASKED, SDC, DETECTED, CRASH, HANG };

struct Result {
    Ending ending;
    bool got_out;
    uint32_t out;       // last value written to OUT_PORT
    uint64_t cycles;
    uint32_t out_count; // how many times OUT_PORT was written
    uint64_t out_hash;  // order-sensitive hash of every value written to OUT_PORT
    bool detected;      // the program wrote to DETECT_PORT (self-check fired)
};

const char* ending_to_string(Ending e) {
    switch (e) {
        case Ending::TRAP: return "TRAP";
        case Ending::BUS_ERROR: return "BUS_ERROR";
        case Ending::TIMEOUT: return "TIMEOUT";
    }
    return "UNKNOWN";
}

const char* outcome_to_string(Outcome o) {
    switch (o) {
        case Outcome::MASKED: return "MASKED";
        case Outcome::SDC: return "SDC";
        case Outcome::DETECTED: return "DETECTED";
        case Outcome::CRASH: return "CRASH";
        case Outcome::HANG: return "HANG";
    }
    return "UNKNOWN";
}

/**
 * Order of checks matters:
 *  1. bus error / timeout: the run never finished normally.
 *  2. detected: the program's own self-check fired (e.g. DMR mismatch).
 *  3. no output at all: it trapped without ever producing a result.
 *  4. otherwise compare the *whole* output stream (count + hash), not just
 *     the last value, so duplicated / extra / missing writes count as SDC.
 */
Outcome classify(const Result& r, const Result& golden) {
    if (r.ending == Ending::BUS_ERROR)
        return Outcome::CRASH;

    if (r.ending == Ending::TIMEOUT)
        return Outcome::HANG;

    if (r.detected)
        return Outcome::DETECTED;

    if (!r.got_out)
        return Outcome::CRASH;

    return (r.out_count == golden.out_count && r.out_hash == golden.out_hash)
        ? Outcome::MASKED
        : Outcome::SDC;
}


struct Target {
    const char* name;
    int width; // numero de bits
    int count; // numero ed copias (cpuregs tem 32, o resto 1a)
    std::function<void(Vpicorv32&, int idx, int bit)> flip;
};

std::vector<Target> targets = {
    {"reg_pc",  32, 1,  [](Vpicorv32& t, int, int b){ t.rootp->picorv32__DOT__reg_pc  ^= 1u << b; }}, // program counter
    {"cpuregs", 32, 32, [](Vpicorv32& t, int i, int b){ t.rootp->picorv32__DOT__cpuregs[i] ^= 1u << b; }},
};

/** Cada fault é um conjunto de um registo, 
 * um bit, e em ciclo em que um registo deve ser editado
*/
struct Fault { const Target* tgt; int idx, bit; uint64_t cycle; };

void apply_fault(Vpicorv32& top, const Fault& fault) {
    fault.tgt->flip(top, fault.idx, fault.bit);
}