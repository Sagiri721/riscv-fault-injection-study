#pragma once

#include <cstdint>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>

#include "report.h"
#include "fault.h"
#include "runtime.h"

// A faulty run is declared a HANG once it exceeds golden_cycles * HANG_FACTOR
// (+ slack). Waiting for the global maxCycles instead makes every hang cost
// ~1M simulated cycles, which dominates the campaign runtime.
constexpr uint64_t HANG_FACTOR = 10;
constexpr uint64_t HANG_SLACK  = 100;

struct CampaignStats {
    uint64_t total = 0;
    uint64_t masked = 0;
    uint64_t sdc = 0;
    uint64_t detected = 0;
    uint64_t crash = 0;
    uint64_t hang = 0;
};

bool runCampaign(uint32_t* image, const uint64_t maxCycles, int* interestingRegisters,
                 int registerCount, const char* resultsPath, uint64_t cycleStride = 1) {
    
    if (!interestingRegisters || registerCount <= 0) {
        std::cerr << "fault campaign requires at least one interesting register" << std::endl;
        return false;
    }

    const Target* registerTarget = nullptr;
    for (const auto& target : targets) {
        if (std::strcmp(target.name, "cpuregs") == 0) {
            registerTarget = &target;
            break;
        }
    }

    if (!registerTarget) {
        std::cerr << "fault campaign has no cpuregs target" << std::endl;
        return false;
    }

    for (int i = 0; i < registerCount; ++i) {
        if (interestingRegisters[i] < 0 || interestingRegisters[i] >= registerTarget->count) {
            std::cerr << "interesting register index out of range: "
                      << interestingRegisters[i] << std::endl;
            return false;
        }
    }

    std::ofstream results(resultsPath);
    if (!results) {
        std::cerr << "could not open results CSV for writing: " << resultsPath << std::endl;
        return false;
    }

    reportCsvHeader(results);

    std::cout << "[campaign] Running golden execution..." << std::endl;
    const Result golden = run(image, maxCycles, nullptr, true);
    
    reportGoldenRun(golden);

    if (golden.ending != Ending::TRAP || !golden.got_out || golden.detected) {
        std::cerr << "golden run is not a clean execution (needs: TRAP, at least one "
                     "OUT_PORT write, no DETECT_PORT write)" << std::endl;
        return false;
    }

    if (cycleStride == 0) cycleStride = 1;

    CampaignStats stats;
    uint64_t injection_cycles = golden.cycles;
    const uint64_t faultyMaxCycles = std::min(maxCycles, golden.cycles * HANG_FACTOR + HANG_SLACK);

    std::cout << "[campaign] Testing " << registerCount
              << " interesting registers across " << registerTarget->width
              << " bits each (cycle stride " << cycleStride << ", hang limit "
              << faultyMaxCycles << " cycles)..." << std::endl;

    for (int selected = 0; selected < registerCount; ++selected) {
        const int idx = interestingRegisters[selected];
        for (int bit = 0; bit < registerTarget->width; ++bit) {
            for (uint64_t cycle = 0; cycle < injection_cycles; cycle += cycleStride) {
                    
                Fault fault{
                    .tgt = registerTarget,
                    .idx = idx,
                    .bit = bit,
                    .cycle = cycle
                };

                Result tampered = run(image, faultyMaxCycles, &fault, false);
                Outcome outcome = classify(tampered, golden);

                // reportCompareRun(tampered, golden, fault);
                reportCsvRun(results, fault, tampered, outcome, golden);

                stats.total++;
                switch (outcome) {
                    case Outcome::MASKED: stats.masked++; break;
                    case Outcome::SDC: stats.sdc++; break;
                    case Outcome::DETECTED: stats.detected++; break;
                    case Outcome::CRASH: stats.crash++; break;
                    case Outcome::HANG: stats.hang++; break;
                }
            }
        }
    }

    std::cout << "[campaign] Total Faults Injected:\t" << stats.total << std::endl;
    std::cout << "[campaign] \t- Masked:\t" << stats.masked << " (" << (100.0 * stats.masked / stats.total) << "%)" << std::endl;
    std::cout << "[campaign] \t- SDC:\t" << stats.sdc << " (" << (100.0 * stats.sdc / stats.total) << "%)" << std::endl;
    std::cout << "[campaign] \t- Detected:\t" << stats.detected << " (" << (100.0 * stats.detected / stats.total) << "%)" << std::endl;
    std::cout << "[campaign] \t- Crash:\t" << stats.crash << " (" << (100.0 * stats.crash / stats.total) << "%)" << std::endl;
    std::cout << "[campaign] \t- Hang:\t" << stats.hang << " (" << (100.0 * stats.hang / stats.total) << "%)" << std::endl;
    results.close();
    if (!results) {
        std::cerr << "failed to finish writing results CSV: " << resultsPath << std::endl;
        return false;
    }
    std::cout << "[campaign] Per-run results written to " << resultsPath << std::endl;
    return true;
}