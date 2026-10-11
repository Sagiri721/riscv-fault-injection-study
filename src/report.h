#pragma once

#include <ostream>

#include "fault.h"

void reportCsvHeader(std::ostream& results) {
    results << "target,register_index,bit_index,injection_cycle,outcome,ending,"
               "got_output,output,cycles,golden_ending,golden_got_output,"
               "golden_output,golden_cycles,detected,out_count,out_hash,"
               "golden_out_count,golden_out_hash\n";
}

void reportCsvRun(std::ostream& results, const Fault& fault, const Result& result,
                  Outcome outcome, const Result& golden) {
    results << fault.tgt->name << ','
            << fault.idx << ','
            << fault.bit << ','
            << fault.cycle << ','
            << outcome_to_string(outcome) << ','
            << ending_to_string(result.ending) << ','
            << result.got_out << ','
            << result.out << ','
            << result.cycles << ','
            << ending_to_string(golden.ending) << ','
            << golden.got_out << ','
            << golden.out << ','
            << golden.cycles << ','
            << result.detected << ','
            << result.out_count << ','
            << result.out_hash << ','
            << golden.out_count << ','
            << golden.out_hash << '\n';
}

void reportCompareRun(const Result& result, const Result& golden, Fault& fault) {

    const Outcome outcome = classify(result, golden);

    printf("\nFAULT:\n");
    printf("\tTARGET\t%s[%d]\n", fault.tgt->name, fault.idx);

    printf("\tBIT:\t%d\n", fault.bit);
    printf("\tCYCLE:\t%llu\n", static_cast<unsigned long long>(fault.cycle));

    printf("\nTAMPERED:\n");
    printf("\tOUT:\t0x%08x\n", result.out);
    printf("\tENDING:\t%s\n", ending_to_string(result.ending));
    printf("\tCYCLES:\t%llu\n", static_cast<unsigned long long>(result.cycles));
    printf("\tOUTCOME\t%s\n", outcome_to_string(outcome));
}

void reportGoldenRun(const Result& result) {

    printf("GOLDEN:\n");
    printf("\tOUT:\t0x%08x\n", result.out);
    printf("\tENDING:\t%s\n", ending_to_string(result.ending));
    printf("\tCYCLES:\t%llu\n", static_cast<unsigned long long>(result.cycles));
}