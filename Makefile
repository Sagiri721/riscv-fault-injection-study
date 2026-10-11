PROGRAM ?= programs/sum
RISCV_GCC ?= riscv64-unknown-elf-gcc
RISCV_OBJCOPY ?= riscv64-unknown-elf-objcopy
VERILATOR ?= verilator
BUILD_DIR ?= build
RESULTS ?= debug/$(notdir $(PROGRAM)).csv
# Registers to inject into: taken from a "# regs: 5 6 7" line in the program's file
REGS ?= $(shell sed -n 's/^\#[[:space:]]*regs:[[:space:]]*//p' $(PROGRAM).s 2>/dev/null)
# Stride to inject on
STRIDE ?= 1
PROGRAMS := $(basename $(wildcard programs/*.s))
NAMES := $(notdir $(PROGRAMS))
# Parallel jobs for run_all
JOBS ?= $(shell nproc 2>/dev/null || echo 1)

RTL := vendor/picorv32/picorv32.v
HARNESS_SRC := src/harness.cpp
HARNESS := $(BUILD_DIR)/obj_dir/Vpicorv32

.PHONY: all run run_all summary build_program build_harness clean

all: build_program build_harness

run: build_program build_harness
	mkdir -p "$(dir $(RESULTS))"
	$(HARNESS) $(PROGRAM).bin $(RESULTS) --regs="$(REGS)" --stride=$(STRIDE) --vcd=debug/$(notdir $(PROGRAM)).vcd

# Run every program in concurrently
run_all: build_harness
	@$(MAKE) --no-print-directory -Otarget -j$(JOBS) $(addprefix run/,$(NAMES))
	@$(MAKE) --no-print-directory summary

run/%:
	@echo "=== programs/$* ==="
	@$(MAKE) --no-print-directory run PROGRAM=programs/$*

summary:
	@printf "%-16s %9s %8s %8s %9s %8s %8s\n" program runs masked sdc detected crash hang
	@awk -F, 'FNR==1 {next} \
		{ f=FILENAME; sub(".*/","",f); sub("\\.csv$$","",f); n[f]++; c[f,$$5]++ } \
		END { \
		      for (f in n) printf "%-16s %9d %7.2f%% %7.2f%% %8.2f%% %7.2f%% %7.2f%%\n", f, n[f], \
		          100*c[f,"MASKED"]/n[f], 100*c[f,"SDC"]/n[f], 100*c[f,"DETECTED"]/n[f], \
		          100*c[f,"CRASH"]/n[f], 100*c[f,"HANG"]/n[f] }' \
		$(addprefix debug/,$(addsuffix .csv,$(NAMES))) | sort

build_harness: $(HARNESS)

$(BUILD_DIR):
	mkdir -p $@

$(HARNESS): $(RTL) $(HARNESS_SRC) src/fault.h src/runtime.h src/report.h src/campaign.h Makefile | $(BUILD_DIR)
	$(VERILATOR) --cc $(RTL) --top-module picorv32 --Mdir $(BUILD_DIR)/obj_dir \
		--exe $(abspath $(HARNESS_SRC)) --build -O3 -Wno-fatal \
		-GSTACKADDR=65536 -GREGS_INIT_ZERO=1 --public-flat-rw --trace

build_program: $(PROGRAM).bin

$(PROGRAM).elf: $(PROGRAM).s
	$(RISCV_GCC) -march=rv32i -mabi=ilp32 -nostdlib -nostartfiles \
		-Wl,-Ttext=0 -o $@ $<

$(PROGRAM).bin: $(PROGRAM).elf
	$(RISCV_OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR) obj_dir
	rm -f programs/*.elf programs/*.bin debug/*.vcd debug/*.csv
