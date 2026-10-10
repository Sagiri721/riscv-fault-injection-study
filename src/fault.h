#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "Vpicorv32.h"
#include "Vpicorv32___024root.h"

enum class Ending  { TRAP, BUS_ERROR, TIMEOUT };
enum class Outcome { MASKED, SDC, CRASH, HANG };

struct Result {
    Ending ending;
    bool got_out;
    uint32_t out;
    uint64_t cycles;
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
        case Outcome::CRASH: return "CRASH";
        case Outcome::HANG: return "HANG";
    }
    return "UNKNOWN";
}

Outcome classify(const Result& r, const Result& golden) {
    if (r.ending == Ending::BUS_ERROR)
        return Outcome::CRASH;

    if (r.ending == Ending::TIMEOUT)
        return Outcome::HANG;

    if (!r.got_out)
        return Outcome::CRASH;

    return (r.out == golden.out)
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
    {"reg_op1", 32, 1,  [](Vpicorv32& t, int, int b){ t.rootp->picorv32__DOT__reg_op1 ^= 1u << b; }}, // internal temporary register
    {"cpuregs", 32, 32, [](Vpicorv32& t, int i, int b){ t.rootp->picorv32__DOT__cpuregs[i] ^= 1u << b; }},
};

/** Cada fault é um conjunto de um registo, 
 * um bit, e em ciclo em que um registo deve ser editado
*/
struct Fault { const Target* tgt; int idx, bit; uint64_t cycle; };

void apply_fault(Vpicorv32& top, const Fault& fault) {
    fault.tgt->flip(top, fault.idx, fault.bit);
}